#include <my_global.h>
#include <mysql.h>
#include <string.h>

#include "fds_commons.h"

#define TVC_MAX_QUERY_STR_SZ   512

#define TVC_META_OFFSET_HD 0
#define TVC_META_OFFSET_TAIL 4
#define TVC_META_OFFSET_MAXFS 8


typedef struct __tvc_db_t {

  volid_t vol_id;
  char     *db_name;
  char     *file_path;
  fds_uint32_t max_file_sz;
  fds_uint32_t current_file_sz;
  fds_uint32_t current_head;
  fds_uint32_t current_tail;
  fds_uint64_t last_chkpt_time;
  fds_uint64_t head_chkpt_time;
  MYSQL    *db_con; 
  int      fd; // file descriptor
  int      meta_fd; // file descriptor to store meta data like head and tail.
#ifdef TVC_DBG
  FILE     *dbg_fp;
#endif

} tvc_db_t;

typedef struct __tvc_jrnl_entry {

  fds_uint32_t      rel_timestamp;
  fds_uint32_t      txn_id;
  fds_uint16_t      segment_id;
  doid_t        doid;
  unsigned char blk_name_sz;
  unsigned char txn_status;
  char          pad[0]; // Make it a multiple of 4 bytes till here
  char          blk_name[0];

} tvc_jrnle_t;


typedef struct __tvc_chkpt_info {

  fds_uint64_t  timestamp;
  fds_uint32_t  tvc_file_offset;
  volid_t   tpc_vol_id;

} tvc_chkpt_info_t;


//utils

static __inline__ tvc_db_t *tdb_create(int vol_id, const char *db_name, const char *file_path, unsigned int max_file_sz) {

  tvc_db_t *tdb = (tvc_db_t *)malloc(sizeof(tvc_db_t));
  memset(tdb, 0, sizeof(tvc_db_t));
  tdb->vol_id = vol_id;
  tdb->db_name = (char *)malloc(strlen(db_name));
  strcpy(tdb->db_name, db_name);
  tdb->file_path = (char *)malloc(strlen(file_path) + 1);
  strcpy(tdb->file_path, file_path);
  tdb->max_file_sz = max_file_sz;
  tdb->current_file_sz = 0;
  tdb->current_head = 0;
  tdb->current_tail = 0;
  tdb->last_chkpt_time = 0;
  tdb->head_chkpt_time = 0;
  return (tdb);

}

static __inline__ int tvc_db_connect(tvc_db_t *tdb) {

  tdb->db_con = mysql_init(NULL);

  if (tdb->db_con == NULL) {
    fprintf(stderr, "%s\n", mysql_error(tdb->db_con));
    return(-1);
  }

  if (mysql_real_connect(tdb->db_con, "localhost", "root", "", 
			 tdb->db_name, 0, NULL, 0) == NULL) {
    fprintf(stderr, "%s\n", mysql_error(tdb->db_con));
    mysql_close(tdb->db_con);
    return(-1);
  }

  return (0);

}

static __inline__ void store_in_byte_array(unsigned char *byte_array, unsigned int value, int num_bytes) { 

  int i;
  for (i = 0; i < num_bytes; i++) {
    byte_array[num_bytes-i-1] = (value & (0xff << 8*(num_bytes-i-1))) >> 8*(num_bytes-i-1);
  }
  return;
}

static __inline__ unsigned int read_from_byte_array(unsigned char *byte_array, int num_bytes) {

  int i;
  unsigned int value;

  value = 0;
  for (i = num_bytes-1; i >= 0; i--) {
    value = (value << 8) | byte_array[i];
  }
  return value;

}

static __inline__ int retrieve_attr_word_at_offset(tvc_db_t *tdb, unsigned int offset, unsigned int *value) {
  unsigned char byte_array[4];
  
  lseek(tdb->meta_fd, offset, SEEK_SET);
  if (read(tdb->meta_fd, byte_array, 4) < 0) {
    return (-1);
  }
  *value = read_from_byte_array(byte_array, 4);
  return (0);

}

static __inline__ int offset_in_valid_range(tvc_db_t *tdb, unsigned int offset) {

  if (tdb->current_tail > tdb->current_head) {
    return (((offset >= tdb->current_head) && (offset < tdb->current_tail)));
  }
  // tail is above head, rolled over
  return (!(offset >= tdb->current_tail && (offset < tdb->current_head)));

}

static __inline__ int tvc_update_head(tvc_db_t *tdb, unsigned int head) {
  unsigned char head_bytes[4];

  store_in_byte_array(head_bytes, head, 4);
  lseek(tdb->meta_fd, TVC_META_OFFSET_HD, SEEK_SET);
  if (write(tdb->meta_fd, head_bytes, 4) < 0) {
      return (-1);
  }
  tdb->current_head = head;
#ifdef TVC_DBG
  fprintf(tdb->dbg_fp, "%s Update: %lu\n", "HEAD", head);
#endif
  return (0);

}

static __inline__ int tvc_update_tail(tvc_db_t *tdb, unsigned int tail) {

  unsigned char tail_bytes[4];

  store_in_byte_array(tail_bytes, tail, 4);
  lseek(tdb->meta_fd, TVC_META_OFFSET_TAIL, SEEK_SET);
  if (write(tdb->meta_fd, tail_bytes, 4) < 0) {
      return (-1);
  }
  tdb->current_tail = tail;
#ifdef TVC_DBG
  fprintf(tdb->dbg_fp, "%s Update: %lu\n", "TAIL", tail);
#endif
  return (0);
}


static __inline__ int tvc_update_maxfilesize(tvc_db_t *tdb, unsigned int max_file_size) {

  unsigned char fs_bytes[4];

  store_in_byte_array(fs_bytes, max_file_size, 4);
  lseek(tdb->meta_fd, TVC_META_OFFSET_MAXFS, SEEK_SET);
  if (write(tdb->meta_fd, fs_bytes, 4) < 0) {
      return (-1);
  }
  tdb->max_file_sz = max_file_size;
#ifdef TVC_DBG
  fprintf(tdb->dbg_fp, "%s Update: %lu - %x %x %x %x\n", "MAX_FILE_SZ", max_file_size, fs_bytes[0], fs_bytes[1], fs_bytes[2], fs_bytes[3]);
#endif
  return (0);
}

static  __inline__ int tvc_retrieve_head(tvc_db_t *tdb) {
  
  if (retrieve_attr_word_at_offset(tdb, TVC_META_OFFSET_HD, &tdb->current_head) < 0) {
    return (-1);
  }
#ifdef TVC_DBG
  fprintf(tdb->dbg_fp, "%s Retrieve: %lu\n", "HEAD",tdb->current_head);
#endif
  return (0);

}


static  __inline__ int tvc_retrieve_tail(tvc_db_t *tdb) {
  
  if (retrieve_attr_word_at_offset(tdb, TVC_META_OFFSET_TAIL, &tdb->current_tail) < 0) {
    return (-1);
  }
#ifdef TVC_DBG
  fprintf(tdb->dbg_fp, "%s Retrieve: %lu\n", "TAIL",tdb->current_tail);
#endif
  return (0);

}

static  __inline__ int tvc_retrieve_maxfilesize(tvc_db_t *tdb) {
  
  if (retrieve_attr_word_at_offset(tdb, TVC_META_OFFSET_MAXFS, &tdb->max_file_sz) < 0) {
    return (-1);
  }
#ifdef TVC_DBG
  fprintf(tdb->dbg_fp, "%s Retrieve: %lu\n", "MAX_FILE_SZ",tdb->max_file_sz);
#endif
  return (0);

}



static __inline__ tvc_jrnle_t *jrnl_entry_alloc(const char *blk_name, fds_uint32_t txn_id, int segment_id, const doid_t doid, fds_uint32_t rel_time) {

  tvc_jrnle_t *jrnle;
  int blk_name_sz;

  blk_name_sz = strlen(blk_name) + 1;
  
  if (blk_name_sz >= 256) {
    return (NULL);
  }

  jrnle = (tvc_jrnle_t *)malloc(blk_name_sz + sizeof(tvc_jrnle_t));
  jrnle->txn_id = txn_id;
  jrnle->rel_timestamp = rel_time;
  jrnle->segment_id = segment_id;
  memcpy(&jrnle->doid[0], &doid[0], sizeof(doid_t));
  jrnle->blk_name_sz = blk_name_sz;
  strcpy(&jrnle->blk_name[0], blk_name);
  return (jrnle);

}

static __inline__ int fill_tvc_entry(tvce_t *tvc_entry, tvc_jrnle_t *jrnle, char *jrnl_blk_name, fds_uint64_t jrnl_timestamp) {
  tvc_entry->timestamp = jrnl_timestamp;
  tvc_entry->blk_name = jrnl_blk_name;
  tvc_entry->segment_id = jrnle->segment_id;
  memcpy(tvc_entry->doid, jrnle->doid, sizeof(doid_t));
  return (0);
}

static __inline__ int tvc_chkpt_delete(tvc_db_t *tdb, fds_uint64_t basetime) {

  char *tmp_query_string;

  tmp_query_string = (char *)malloc(TVC_MAX_QUERY_STR_SZ);

  // Ideally we should delete this row if TPCVolID = NULL
  sprintf(tmp_query_string, "UPDATE TVC_CHKPT_IDX SET TVCFileOffset = NULL WHERE VolID = %d AND BaseTime = %llu;",
	  tdb->vol_id, basetime);

#ifdef TVC_DBG
  printf("Query string: %s\n", tmp_query_string);
  fprintf(tdb->dbg_fp, "DELETING CHKPT at timestamp - %llu", basetime);
#endif

  if (mysql_query(tdb->db_con, tmp_query_string)) {
    fprintf(stderr, "%s\n", mysql_error(tdb->db_con));
    free(tmp_query_string);
    return(-1);
  }

  free(tmp_query_string);
  return (0);

}

static int tvc_delete_chkpts_outside_valid_range(tvc_db_t *tdb) {

  char *tmp_query_string;
  MYSQL_RES *res;
  MYSQL_ROW a_row;
  unsigned int num_chkpts = 0;

  tmp_query_string = (char *)malloc(TVC_MAX_QUERY_STR_SZ);

  if (tdb->current_tail > tdb->current_head) {
      sprintf(tmp_query_string, "SELECT BaseTime, TVCFileOffset FROM TVC_CHKPT_IDX WHERE VolID = %d AND (TVCFileOffset < %lu OR TVCFileOffset >= %lu);",
    tdb->vol_id, tdb->current_head, tdb->current_tail);
  } else {
    sprintf(tmp_query_string, "SELECT BaseTime, TVCFileOffset FROM TVC_CHKPT_IDX WHERE VolID = %d AND TVCFileOffset >= %lu AND TVCFileOffset < %lu;",
    tdb->vol_id, tdb->current_tail, tdb->current_head);
  }
      
#ifdef TVC_DBG
  printf("Query string: %s\n", tmp_query_string);
#endif

  if (mysql_query(tdb->db_con, tmp_query_string)) {
    fprintf(stderr, "%s\n", mysql_error(tdb->db_con));
    free(tmp_query_string);
    return(-1);
  }  
  res = mysql_store_result(tdb->db_con);
  if (res == NULL) {
    fprintf(stderr, "%s\n", mysql_error(tdb->db_con));
    free(tmp_query_string);
    return(-1);
  }
  num_chkpts = mysql_num_rows(res);
#ifdef TVC_DBG
  fprintf(tdb->dbg_fp, "Invalidating %d checkpoints outside valid range\n", num_chkpts);
#endif
  a_row = mysql_fetch_row(res);
  while (a_row) {
      unsigned int chkpt_offset;
      fds_uint64_t chkpoint_time;

      chkpt_offset = strtoul((const char *)a_row[1], 0, 0);
      chkpoint_time = strtoull((const char *)a_row[0], 0, 0);
      tvc_chkpt_delete(tdb, chkpoint_time);
      a_row = mysql_fetch_row(res);
  }

  mysql_free_result(res);
  free(tmp_query_string);
  return (0);

}

#define DIR_STRICTLY_BEFORE 0
#define DIR_BEFORE 1
#define DIR_AT 2
#define DIR_AFTER 3
#define DIR_STRICTLY_AFTER 4 
#define DIR_LATEST 5

char *cmp_string[5] = {"<", "<=", "=", ">=", ">"};
char *sort_string[5] = {"DESC", "DESC", "DESC", "ASC", "ASC"};


static int tvc_get_next_chkpoint_wrt_offset(tvc_db_t *tdb, unsigned int ref_offset, int direction,
					    int *next_chkpoint_found, fds_uint64_t *next_chkpoint_time, unsigned int *next_chkpoint_offset) {

  char *tmp_query_string;
  MYSQL_RES *res;
  MYSQL_ROW a_row;
  fds_uint32_t chkpt_offset;
  char *cmp_str;

  tmp_query_string = (char *)malloc(TVC_MAX_QUERY_STR_SZ);

  sprintf(tmp_query_string, "SELECT BaseTime, TVCFileOffset FROM TVC_CHKPT_IDX WHERE VolID = %d AND TVCFileOffset %s %lu ORDER BY TVCFileOffset %s;",
	  tdb->vol_id, cmp_string[direction], tdb->current_head, sort_string[direction]);

#ifdef TVC_DBG
  printf("Query string: %s\n", tmp_query_string);
#endif

  if (mysql_query(tdb->db_con, tmp_query_string)) {
    fprintf(stderr, "%s\n", mysql_error(tdb->db_con));
    free(tmp_query_string);
    return(-1);
  }  
  res = mysql_use_result(tdb->db_con);
  if (res == NULL) {
    fprintf(stderr, "%s\n", mysql_error(tdb->db_con));
    free(tmp_query_string);
    return(-1);
  }
  a_row = mysql_fetch_row(res);
  if (!a_row) {
    *next_chkpoint_found = 0;
    mysql_free_result(res);
    free(tmp_query_string);
    return (0);
  }
  chkpt_offset = strtoul((const char *)a_row[1], 0, 0);
  if (!offset_in_valid_range(tdb, chkpt_offset)){
    //something terribly wrong! This portion of the log file is invalid.
    printf("Next checkpoint offset %lu found after tail %lu\n",
	   chkpt_offset, tdb->current_tail);
    mysql_free_result(res);
    free(tmp_query_string);
    return (-1);
  } 
  *next_chkpoint_offset = chkpt_offset;
  *next_chkpoint_time = strtoull((const char *)a_row[0], 0, 0);
  *next_chkpoint_found = 1;
  mysql_free_result(res);
  free(tmp_query_string);
  return (0);

}


static int tvc_get_next_chkpoint_in_time(tvc_db_t *tdb, fds_uint64_t ref_time, int direction, int *next_chkpoint_found,
    fds_uint64_t *next_chkpoint_time, unsigned int *next_chkpoint_offset) {

  char *tmp_query_string;
  MYSQL_RES *res;
  MYSQL_ROW a_row;
  fds_uint32_t chkpt_offset;
  char *cmp_str, *sort_str;
  fds_uint64_t reference_time;

  tmp_query_string = (char *)malloc(TVC_MAX_QUERY_STR_SZ);

  if (direction == DIR_LATEST) {
    cmp_str = ">";
    sort_str = "DESC";
    reference_time = 0;
  } else {
    cmp_str = cmp_string[direction];
    sort_str = sort_string[direction];
    reference_time = ref_time;
  }
  sprintf(tmp_query_string, "SELECT BaseTime, TVCFileOffset FROM TVC_CHKPT_IDX WHERE VolID = %d AND BaseTime %s %llu AND TVCFileOffset >= 0 ORDER BY BaseTime %s;",
	  tdb->vol_id, cmp_str, reference_time, sort_str);

#ifdef TVC_DBG
  printf("Query string: %s\n", tmp_query_string);
#endif

  if (mysql_query(tdb->db_con, tmp_query_string)) {
    fprintf(stderr, "%s\n", mysql_error(tdb->db_con));
    free(tmp_query_string);
    return(-1);
  }  
  res = mysql_use_result(tdb->db_con);
  if (res == NULL) {
    fprintf(stderr, "%s\n", mysql_error(tdb->db_con));
    free(tmp_query_string);
    return(-1);
  }
  a_row = mysql_fetch_row(res);
  if (!a_row) {
    *next_chkpoint_found = 0;
    mysql_free_result(res);
    free(tmp_query_string);
    return (0);
  }
  chkpt_offset = strtoul((const char *)a_row[1], 0, 0);
  if (!offset_in_valid_range(tdb, chkpt_offset)) {
    //something terribly wrong! This portion of the log file is invalid.
    printf("Next checkpoint offset %lu found after tail %lu\n",
	   chkpt_offset, tdb->current_tail);
    mysql_free_result(res);
    free(tmp_query_string);
    return (-1);
  } 
  *next_chkpoint_offset = chkpt_offset;
  *next_chkpoint_time = strtoull((const char *)a_row[0], 0, 0);
  *next_chkpoint_found = 1;
  mysql_free_result(res);
  free(tmp_query_string);
  return (0);
}

#define tvc_get_first_chkpt_after_time(tdb, start_time, p_chkpt_found, p_chkpt_time, p_chkpt_offset) \
  tvc_get_next_chkpoint_in_time(tdb, start_time, DIR_STRICTLY_AFTER, p_chkpt_found, p_chkpt_time, p_chkpt_offset)

#define tvc_get_last_chkpt_before_or_at_time(tdb, end_time, p_chkpt_found, p_chkpt_time, p_chkpt_offset) \
  tvc_get_next_chkpoint_in_time(tdb, end_time, DIR_BEFORE, p_chkpt_found, p_chkpt_time, p_chkpt_offset)

#define tvc_get_latest_chkpt_in_time(tdb, p_chkpt_found, p_chkpt_time, p_chkpt_offset) \
  tvc_get_next_chkpoint_in_time(tdb, 0, DIR_LATEST, p_chkpt_found, p_chkpt_time, p_chkpt_offset)

#define tvc_get_next_chkpoint_after_or_at_offset(tdb, start_offset, pnext_chkpt_found, pnext_chkpt_time, pnext_chkpt_offset) \
  tvc_get_next_chkpoint_wrt_offset(tdb, start_offset, DIR_AFTER, pnext_chkpt_found, pnext_chkpt_time, pnext_chkpt_offset)

#define tvc_get_next_chkpoint_before_or_at_offset(tdb, end_offset, pnext_chkpt_found, pnext_chkpt_time, pnext_chkpt_offset) \
  tvc_get_next_chkpoint_wrt_offset(tdb, end_offset, DIR_BEFORE, pnext_chkpt_found, pnext_chkpt_time, pnext_chkpt_offset)
  
#define tvc_get_chkpoint_at_offset(tdb, start_offset, pnext_chkpt_found, pnext_chkpt_time, pnext_chkpt_offset) \
  tvc_get_next_chkpoint_wrt_offset(tdb, start_offset, DIR_AT, pnext_chkpt_found, pnext_chkpt_time, pnext_chkpt_offset)

static __inline__ tvc_get_basetime_for_offset(tvc_db_t *tdb, unsigned int offset, int *chkpt_found, fds_uint64_t *chkpt_time){

  unsigned int chkpt_offset;

  if (tvc_get_next_chkpoint_before_or_at_offset(tdb, offset, chkpt_found, chkpt_time, &chkpt_offset) < 0){
    return (-1);
  }
  if ((!chkpt_found) && (tdb->current_head > offset)) {
       if (tvc_get_next_chkpoint_before_or_at_offset(tdb, tdb->current_tail, chkpt_found, chkpt_time, &chkpt_offset) < 0){
	 return (-1);
       }
  }
  return (0);

}

static __inline__ int tvc_retrieve_chkpoint_times(tvc_db_t *tdb){

   unsigned int chkpt_offset;
   int chkpt_found;
   fds_uint64_t chkpt_time;

   if (tvc_get_chkpoint_at_offset(tdb, tdb->current_head, &chkpt_found, &chkpt_time, &chkpt_offset) < 0){
     printf("Unable to retrieve head check point\n");
     return (-1);
   }
   tdb->head_chkpt_time = chkpt_time;
#ifdef TVC_DBG
   fprintf(tdb->dbg_fp, "Head Chkpoint Retrieve: %llu\n", chkpt_time);
#endif

   if (tvc_get_latest_chkpt_in_time(tdb, &chkpt_found, &chkpt_time, &chkpt_offset) < 0){
     printf("Unable to retrieve latest check point\n");
     return (-1);
   }
   tdb->last_chkpt_time = chkpt_time;
#ifdef TVC_DBG
   fprintf(tdb->dbg_fp, "Last Chkpoint Retrieve: %llu\n", chkpt_time);
#endif
   return (0);

}

