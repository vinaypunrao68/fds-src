#include <string.h>
#include "fds_commons.h"
#include "tvclib.h"

#include "tvc_private.h"

static int tvc_crt_checkpoint(tvc_db_t *tdb, uint64_t timestamp, volid_t tpc_vol_id, unsigned int file_offset);
static int tvc_move_head(tvc_db_t *tdb, unsigned int after_offset); 

tvc_vhdl_t tvc_vol_create(volid_t vol_id, const char *db_name, const char *file_path, unsigned int max_file_sz) {

  tvc_db_t *tdb = tdb_create(vol_id, db_name, file_path, max_file_sz);
  char *meta_filepath;

  meta_filepath = (char *)malloc(strlen(file_path) + 5 + 1);
  sprintf(meta_filepath, "%s.meta", file_path);

  tdb->fd = open(file_path, O_RDWR | O_CREAT | O_SYNC | O_EXCL);
  if (tdb->fd < 0) {
    perror("tvclib could not open specified file: ");
    free(meta_filepath);
    return (0);
  }

  tdb->meta_fd = open(meta_filepath, O_RDWR | O_CREAT | O_SYNC | O_EXCL);
  if (tdb->meta_fd < 0) {
    perror("tvclib could not open specified file: ");
    free(meta_filepath);
    return (0);
  }

#ifdef TVC_DBG
  sprintf(meta_filepath, "%s.dbg", file_path);
  tdb->dbg_fp = fopen(meta_filepath, "w");
#endif

  if (tvc_db_connect(tdb) < 0) {
    free(meta_filepath);
    return (0);
  }
  tvc_update_head(tdb, 0);
  tvc_update_tail(tdb, 0);
  tvc_update_maxfilesize(tdb, max_file_sz);
  free(meta_filepath);
  return ((tvc_vhdl_t)tdb);
  
}

tvc_vhdl_t tvc_vol_load(volid_t vol_id, const char *db_name, const char *file_path) {

  tvc_db_t *tdb = tdb_create(vol_id, db_name, file_path, 0);
  char *meta_filepath;
  struct stat stat_buf;

  meta_filepath = (char *)malloc(strlen(file_path) + 5 + 1);
  sprintf(meta_filepath, "%s.meta", file_path);

  tdb->fd = open(file_path, O_RDWR | O_SYNC);
  if (tdb->fd < 0) {
    perror("tvclib could not open specified file: ");
    free(meta_filepath);
    return (0);
  }

  if (fstat(tdb->fd, &stat_buf) < 0) {
    perror("tvclib could not open specified file: ");
    free(meta_filepath);
    return (0);
  }
  tdb->current_file_sz = stat_buf.st_size;

  tdb->meta_fd = open(meta_filepath, O_RDWR | O_SYNC);
  if (tdb->meta_fd < 0) {
    free(meta_filepath);
    perror("tvclib could not open specified file: ");
    return (0);
  }

#ifdef TVC_DBG
  sprintf(meta_filepath, "%s.dbg", file_path);
  tdb->dbg_fp = fopen(meta_filepath, "w");
#endif

  if (tvc_db_connect(tdb) < 0) {
    free(meta_filepath);
    return (0);
  }

  tvc_retrieve_head(tdb);
  tvc_retrieve_tail(tdb);
  tvc_retrieve_maxfilesize(tdb);
  free(meta_filepath);

  // Get all checkpoints between original head and new head and invalidate them
  if (tvc_delete_chkpts_outside_valid_range(tdb) < 0) {
    return (0);
  }

  // Now update head_chkpt_time and last_chkpt_time
  if (tvc_retrieve_chkpoint_times(tdb) < 0) {
    return (0);
  }
  return ((tvc_vhdl_t) tdb);

}

int tvc_entry_append(tvc_vhdl_t vhdl, uint32_t txn_id, uint64_t timestamp, const char *blk_name, int segment_id, const doid_t doid, unsigned int *entry_ref_hint) {

  tvc_db_t    *tdb = (tvc_db_t *)vhdl;
  tvc_jrnle_t *jrnle;
  uint64_t    rel_time, new_chkpt_time;
  unsigned int new_tail, write_offset, write_sz, new_file_sz;
  int should_create_new_head = 0;

  // Pass 0 for relative time for now. We will reset this to the right value later
  // when we know what is the base checkpoint time we will use
  jrnle = jrnl_entry_alloc(blk_name, txn_id, segment_id, doid, 0);
  jrnle->txn_status = FDS_DMGR_TXN_STATUS_OPEN;
  write_sz = jrnle->blk_name_sz + sizeof(tvc_jrnle_t);

  if (tdb->current_tail == tdb->current_file_sz) {
    if (tdb->current_file_sz >= tdb->max_file_sz) {
      // Case: Rolling over
      write_offset = 0;
      new_file_sz = tdb->current_file_sz;
    } else {
      // Case: Increasing the file size
      write_offset = tdb->current_tail;
      new_file_sz = new_tail;
    }
  } else {
    // Case: tail is before EOF, overwrite + possible file sz increase
    write_offset = tdb->current_tail;
    new_file_sz = (new_tail > tdb->current_file_sz)? new_tail:tdb->current_file_sz;
  }
  new_tail = write_offset + write_sz;

  if (tdb->current_file_sz > 0) {
    if ((tdb->current_head >= write_offset) && (tdb->current_head < new_tail)){
      // find the next chkpt time after new_tail, before current_tail.
      // If no good chkpt exists to move to, the callee creates one.
      if (tvc_move_head(tdb, new_tail)) {
	return (-1);
      }
    }
  }

  rel_time = timestamp - tdb->last_chkpt_time;

  if (!(tdb->last_chkpt_time) || (rel_time >= FDS_UINT32_MAX)) {
    if (tvc_crt_checkpoint(tdb, timestamp, INV_VOL_ID, write_offset)) {
      return (-1);
    }
    // now tdb->last_chkpt_time would be tvce->timestamp);
    tdb->last_chkpt_time = timestamp;
    rel_time = 0;
  }
  jrnle->rel_timestamp = rel_time;

#ifdef TVC_DBG
  fprintf(tdb->dbg_fp, "Journel Entry: (rel_timestamp - %x, seg id - %d, doid - %s, blk_name_sz - %d, blk_name - %s),  write_sz - %lu, write_offset - %lu\n",
	  jrnle->rel_timestamp, jrnle->segment_id, jrnle->doid, jrnle->blk_name_sz, jrnle->blk_name, write_sz, write_offset);
#endif
  // Append the entry to the log file
  lseek(tdb->fd, write_offset, SEEK_SET);
  write(tdb->fd, jrnle, write_sz);
  *entry_ref_hint = write_offset;

  tvc_update_tail(tdb, new_tail);
  tdb->current_file_sz = new_file_sz;
  
  free(jrnle);
  return (0);

}

int tvc_entry_status_update(tvc_vhdl_t hdl, uint32_t txn_id, int64_t entry_ref_hint, unsigned char status) {

  tvc_db_t *tdb = (tvc_db_t *)hdl;
  unsigned int file_offset;
  tvc_jrnle_t jrnle;

  if (entry_ref_hint < 0) {
    // We have to search the whole log file for this txn! 
    // Return error for now.
    return (-1);
  }

  file_offset = (unsigned int)entry_ref_hint;
  if (!(offset_in_valid_range(tdb, file_offset))) {
    // We do not have that entry any more! The log has rolled over!
    return (-1);
  }

  // Now get journal entry at this offset
  lseek(tdb->fd, file_offset, SEEK_SET);
  read(tdb->fd, &jrnle, sizeof(tvc_jrnle_t));  

  // Sanity check on the txn id
  if (jrnle.txn_id != txn_id) {
    return (-1);
  }
  // Update status
  jrnle.txn_status = status;
  // Write back the journal entry
  lseek(tdb->fd, file_offset, SEEK_SET);
  write(tdb->fd, &jrnle, sizeof(tvc_jrnle_t));

  return (0);

}

int tvc_crt_checkpoint(tvc_db_t *tdb, uint64_t timestamp, volid_t tpc_vol_id, unsigned int file_offset) {

  char *tmp_query_string;

  tmp_query_string = (char *)malloc(TVC_MAX_QUERY_STR_SZ);
  if (tpc_vol_id == INV_VOL_ID) {
    sprintf(tmp_query_string, "INSERT INTO TVC_CHKPT_IDX (VolID, BaseTime, TVCFileOffset) VALUES (%d, %llu, %lu);",
	    tdb->vol_id, timestamp, file_offset);
  } else {
    sprintf(tmp_query_string, "INSERT INTO TVC_CHKPT_IDX (VolID, BaseTime, TVCFileOffset, TPCVolID) VALUES (%d, %llu, %lu, %d);",
	    tdb->vol_id, timestamp, file_offset, tpc_vol_id);
  }
#ifdef TVC_DBG
  printf("Query string: %s\n", tmp_query_string);
#endif
  if (mysql_query(tdb->db_con, tmp_query_string)) {
    fprintf(stderr, "%s\n", mysql_error(tdb->db_con));
    free(tmp_query_string);
    return(-1);
  }
#ifdef TVC_DBG
  fprintf(tdb->dbg_fp, "Creating Checkpoint: vol_id - %d, basetime - %llx, offset - %lu, tpc_vol - %d\n",
	  tdb->vol_id, timestamp, file_offset, tpc_vol_id);
#endif
  free(tmp_query_string);
  return (0);

}

static int tvc_move_head(tvc_db_t *tdb, unsigned int start_offset) {

  uint64_t orig_head_chkpoint_time, next_chkpt_time;
  unsigned int next_chkpt_offset, orig_head_offset;
  int next_chkpt_found;

  orig_head_chkpoint_time = tdb->head_chkpt_time;
  orig_head_offset = tdb->current_head;

  // Assumption: the callee will never return a chkpoint outside the valid range. 
  if (tvc_get_next_chkpoint_after_or_at_offset(tdb, start_offset, &next_chkpt_found, &next_chkpt_time, &next_chkpt_offset)) {
    return (-1);
  }
  if ((!next_chkpt_found) && (tdb->current_tail < start_offset)) {
    // tail rolled over
    if (tvc_get_next_chkpoint_after_or_at_offset(tdb, 0, &next_chkpt_found, &next_chkpt_time, &next_chkpt_offset)) {
      return (-1);
    }   
  }

  if (next_chkpt_found) {
    tvc_update_head(tdb, next_chkpt_offset);
    tdb->head_chkpt_time = next_chkpt_time;
  } else {
    tvc_jrnle_t tmp_jrnle;
    unsigned int curr_offset, prev_offset;

    // create a new chkpoint past start_offset and make that the new head
    curr_offset = tdb->current_head;
    while(1) {
      lseek(tdb->fd, curr_offset, SEEK_SET);
      read(tdb->fd, &tmp_jrnle, sizeof(tvc_jrnle_t));
      prev_offset = curr_offset;
      curr_offset += sizeof(tvc_jrnle_t) + tmp_jrnle.blk_name_sz;
      if ((tdb->current_tail > prev_offset) && (tdb->current_tail <= curr_offset)) {
	printf("Something does not add up. Went past tail looking for a new head\n");
	return (-1);
      }
      if ((prev_offset < start_offset) && (curr_offset >= start_offset)) {
	break;
      }
      if (curr_offset == tdb->current_file_sz) {
	curr_offset = 0;
      }
      if (curr_offset > tdb->current_file_sz) {
	printf("Something does not add up. TVC file ended abruptly\n");
	return (-1);
      }
    }
    if (curr_offset == tdb->current_file_sz) {
      curr_offset = 0;
    }
    lseek(tdb->fd, curr_offset, SEEK_SET);
    read(tdb->fd, &tmp_jrnle, sizeof(tvc_jrnle_t));
    if (tvc_get_basetime_for_offset(tdb, curr_offset, &next_chkpt_found, &next_chkpt_time) < 0) {
      printf("Something does not add up. Not finding base time for an entry\n");
      return (-1);
    }
    tdb->head_chkpt_time = next_chkpt_time + tmp_jrnle.rel_timestamp;
    tvc_crt_checkpoint(tdb, tdb->head_chkpt_time, INV_VOL_ID, curr_offset);
    tvc_update_head(tdb, curr_offset);
    tdb->last_chkpt_time = tdb->head_chkpt_time;
  }

  // Note, we may fail before deleting this checkpoint.
  // If we do, when we come up, first thing before accessing anything, we should delete all those checkpoints whose offsets
  // are outside of current (head, tail) range.

  // Get all checkpoints between original head and new head and invalidate them
  if (tvc_delete_chkpts_outside_valid_range(tdb)) {
    return (-1);
  }
  if (tvc_chkpt_delete(tdb, orig_head_chkpoint_time)) {
    return (-1);
  }

  return (0);
 
}

int tvc_mark_checkpoint(tvc_vhdl_t vhdl, uint64_t timestamp, volid_t tpc_vol_id) {

  tvc_db_t *tdb = (tvc_db_t *)vhdl;
  unsigned int file_offset;

  if ((tdb->current_tail == tdb->current_file_sz) && (tdb->current_file_sz >= tdb->max_file_sz)) {
    file_offset = 0;
  } else {
    file_offset = tdb->current_tail;
  }
  if (tvc_crt_checkpoint(tdb, timestamp, tpc_vol_id, file_offset)) {
    return (-1);
  }
  tdb->last_chkpt_time = timestamp;
  return (0);
  
}

int tvc_entries_get(tvc_vhdl_t vhdl, uint64_t start_time, int *n_entries, tvce_t *tvc_entries, int *end_of_log) {

  tvc_db_t *tdb = (tvc_db_t *)vhdl;
  uint64_t chkpt_time, next_chkpt_time;
  unsigned int chkpt_offset, next_chkpt_offset, curr_offset, next_rd_offset;
  int chkpt_found, next_chkpt_found, i;

  tvc_get_last_chkpt_before_or_at_time(tdb, start_time, &chkpt_found, &chkpt_time, &chkpt_offset);

  if (!chkpt_found) {
    tvc_get_first_chkpt_after_time(tdb, start_time, &chkpt_found, &chkpt_time, &chkpt_offset);
  }
  if (!chkpt_found) {
    printf("No checkpoint found. Possible corruption in logs\n");
    return (-1);
  }

  if (!offset_in_valid_range(tdb, chkpt_offset)) {
    printf(" Found checkpoint outside valid range\n");
    return (-1);
  }

  tvc_get_first_chkpt_after_time(tdb, chkpt_time, &next_chkpt_found, &next_chkpt_time, &next_chkpt_offset);
  if ((next_chkpt_found) && (!offset_in_valid_range(tdb, next_chkpt_offset))) {
    printf(" Found checkpoint outside valid range\n");
    return (-1);
  }

  curr_offset = chkpt_offset;

  i = 0;
  *end_of_log = 0;

  while(1) {
    uint64_t jrnl_timestamp;
    tvc_jrnle_t tmp_jrnle;
    char    *jrnl_blk_name;

    printf("Reading entry at %lu\n",curr_offset); 
    lseek(tdb->fd, curr_offset, SEEK_SET);
    read(tdb->fd, (char *)&tmp_jrnle, sizeof(tvc_jrnle_t));
    jrnl_timestamp = chkpt_time + tmp_jrnle.rel_timestamp;
    next_rd_offset = curr_offset + sizeof(tvc_jrnle_t) + tmp_jrnle.blk_name_sz;
    if (next_rd_offset == tdb->current_tail) {
      *end_of_log = 1;
    }
    if (next_rd_offset >= tdb->current_file_sz) {
      next_rd_offset = 0;
    }
    if ((next_chkpt_found) && (next_rd_offset >= next_chkpt_offset)) {
      chkpt_time = next_chkpt_time;
      chkpt_offset = next_chkpt_offset;
      tvc_get_first_chkpt_after_time(tdb, chkpt_time, &next_chkpt_found, &next_chkpt_time, &next_chkpt_offset);
      if ((next_chkpt_found) && (!offset_in_valid_range(tdb, next_chkpt_offset))) {
	printf(" Found checkpoint outside valid range\n");
	return (-1);
      }
    }
    if (jrnl_timestamp < start_time) {
      curr_offset = next_rd_offset;
      printf("Skipping entry\n");
      if (*end_of_log) {
	break;
      }
      continue;
    }
    jrnl_blk_name = (char *)malloc(tmp_jrnle.blk_name_sz) + 1;
    read(tdb->fd, jrnl_blk_name, tmp_jrnle.blk_name_sz);
    jrnl_blk_name[tmp_jrnle.blk_name_sz] = 0;
    fill_tvc_entry(&tvc_entries[i], &tmp_jrnle, jrnl_blk_name, jrnl_timestamp);
    //printf("Retrieved entry %d: %lu, %s, %d, %s", i, jrnl_timestamp, jrnl_blk_name, tmp_jrnle.segment_id, tmp_jrnle.doid); 
    i++;
    if (*end_of_log) {
      break;
    }
    if (i == *n_entries) {
      break;
    }
    curr_offset = next_rd_offset;
  }
  *n_entries = i;
  printf("Returning %d entries\n", i);
  return (0);

}
