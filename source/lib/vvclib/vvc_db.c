#include "string.h"

#include "vvc_private.h"
#define VVC_MAX_QUERY_STR_SZ   1024

int vvc_db_connect(vvc_vdb_t *vdb) {

    MYSQL *db_con = mysql_init(NULL);

    if (db_con == NULL) 
    {
      fprintf(stderr, "%s\n", mysql_error(db_con));
      return(-1);
    }

    if (mysql_real_connect(db_con, "localhost", "root", "", 
			   vdb->db_name, 0, NULL, 0) == NULL) 
    {
      fprintf(stderr, "%s\n", mysql_error(db_con));
      mysql_close(db_con);
      return(-1);
    }
    vdb->db_con = db_con;
    return (0);

}

int vvc_db_entry_load(vvc_vdb_t *vdb, const char *blk_name) {

  int i, num_segments;
  doid_t **doid_list;
  char *tmp_query_string;
  MYSQL_RES *all_segs_rslt;
  MYSQL_ROW a_seg_row;
  vvce_t  *vvce;

  if (!vdb->db_name) {
    return (0);
  }

  tmp_query_string = (char *)vvc_malloc(VVC_MAX_QUERY_STR_SZ);

  sprintf(tmp_query_string, "SELECT SegmentID, DOID FROM VVC WHERE VolID = %d AND BlockName = \'%s\' ORDER BY SegmentID ASC;", 
	  vdb->vol_id, blk_name);

  if (mysql_query(vdb->db_con, tmp_query_string)) {
    fprintf(stderr, "%s\n", mysql_error(vdb->db_con));
    vvc_mfree(tmp_query_string);
    return(-1);
  }
  all_segs_rslt= mysql_store_result(vdb->db_con);
  if (all_segs_rslt == NULL) {
    fprintf(stderr, "%s\n", mysql_error(vdb->db_con));
    vvc_mfree(tmp_query_string);
    return(-1);
  }
  num_segments = mysql_num_rows(all_segs_rslt);
  if (num_segments <= 0) {
    return (0);
  }
  doid_list = (doid_t **)vvc_malloc(num_segments * sizeof(doid_t *));
  i = 0;
  while ((a_seg_row = mysql_fetch_row(all_segs_rslt))) {
    if (i >= num_segments) {
      fprintf(stderr, "MYSQL error: more rows than expected\n");
      vvc_mfree(doid_list);
      vvc_mfree(tmp_query_string);
      return(-1);
    }
    doid_list[i++] = (doid_t *)a_seg_row[1];
  }

  vvce = vvce_create(blk_name, num_segments, (const doid_t **)doid_list); 
  lh_insert(vdb->vvc_table, vvce);

  mysql_free_result(all_segs_rslt);
  vvc_mfree(doid_list);
  vvc_mfree(tmp_query_string);

  return(0);

}

int vvc_db_entries_load(vvc_vdb_t *vdb) {

  char *tmp_query_string;
  MYSQL_RES *all_blks_rslt;
  MYSQL_ROW a_blk_row;

  if (!vdb->db_name) {
    return (0);
  }

  tmp_query_string = (char *)vvc_malloc(VVC_MAX_QUERY_STR_SZ);

  sprintf(tmp_query_string, "SELECT DISTINCT BlockName FROM VVC WHERE VolID = %d;", vdb->vol_id);

  if (mysql_query(vdb->db_con, tmp_query_string)) {
    fprintf(stderr, "%s\n", mysql_error(vdb->db_con));
    vvc_mfree(tmp_query_string);
    return(-1);
  }
  all_blks_rslt = mysql_store_result(vdb->db_con);
  if (all_blks_rslt == NULL) {
    fprintf(stderr, "%s\n", mysql_error(vdb->db_con));
    vvc_mfree(tmp_query_string);
    return(-1);
  }
  while ((a_blk_row = mysql_fetch_row(all_blks_rslt))) {
    char *blk_name = a_blk_row[0];
    vvc_db_entry_load(vdb, blk_name);
  }
  mysql_free_result(all_blks_rslt);
  vvc_mfree(tmp_query_string);
  return(0);
  
}

int vvc_db_entry_create(vvc_vdb_t *vdb, vvce_t *vvce) {

  int i;
  char *tmp_query_string;

  if (!vdb->db_name) {
    return (0);
  }

  tmp_query_string = (char *)vvc_malloc(VVC_MAX_QUERY_STR_SZ);
  
  for (i = 0; i < vvce->num_segments; i++) {

    sprintf(tmp_query_string, "INSERT INTO VVC (VolID, BlockName, BlockNameHash, SegmentID, DOID) VALUES (%d, \'%s\', \'%s\', %d, \'%s\');",
	    vdb->vol_id, vvce->blk_name, "NoHashYet", i, vvce->doid_list[i] 
	    );
#ifdef VVC_DBG
    vvc_print("Query string: %s\n", tmp_query_string);
#endif
    if (mysql_query(vdb->db_con, tmp_query_string)) {
      fprintf(stderr, "%s\n", mysql_error(vdb->db_con));
      vvc_mfree(tmp_query_string);
      return(-1);
    }
  }
  vvc_mfree(tmp_query_string);
  return (0);

}

int vvc_db_entry_update(vvc_vdb_t *vdb, vvce_t *prev_vvce, vvce_t *new_vvce) {

  int i;
  int max_segments;
  char *tmp_query_string;

  if (!vdb->db_name) {
    return (0);
  }
  max_segments = (new_vvce->num_segments > prev_vvce->num_segments)?
    new_vvce->num_segments:prev_vvce->num_segments;

  tmp_query_string = (char *)vvc_malloc(VVC_MAX_QUERY_STR_SZ);
  
  for (i = 0; i < max_segments; i++) {

    if (i < new_vvce->num_segments) {

      if (i >= prev_vvce->num_segments) {
	sprintf(tmp_query_string, "INSERT INTO VVC (VolID, BlockName, BlockNameHash, SegmentID, DOID) VALUES (%d, \'%s\', \'%s\', %d, \'%s\');",
		vdb->vol_id, new_vvce->blk_name, "NoHashYet", i, new_vvce->doid_list[i]);
      } else {
	if (strncmp(&prev_vvce->doid_list[i], &new_vvce->doid_list[i], sizeof(doid_t)) == 0) {
	  continue;
	}
	sprintf(tmp_query_string, "UPDATE VVC SET DOID = \'%s\' WHERE VolID = %d AND BlockName = \'%s\' AND SegmentID = %d;",
		new_vvce->doid_list[i], vdb->vol_id, new_vvce->blk_name, i);
      }

    } else if (i < prev_vvce->num_segments) {
      sprintf(tmp_query_string, "DELETE FROM VVC WHERE VolID = %d AND BlockName = \'%s\' AND SegmentID = %d;",
	      vdb->vol_id, prev_vvce->blk_name, i);
    }

#ifdef VVC_DBG
    vvc_print("Query string: %s\n", tmp_query_string);
#endif
    if (mysql_query(vdb->db_con, tmp_query_string)) {
      fprintf(stderr, "%s\n", mysql_error(vdb->db_con));
      vvc_mfree(tmp_query_string);
      return(-1);
    }

  }

  vvc_mfree(tmp_query_string);
  return (0);

}

int vvc_db_entry_delete(vvc_vdb_t *vdb, vvce_t *vvce) {

  char *tmp_query_string;

  if (!vdb->db_name) {
    return (0);
  }

  tmp_query_string = (char *)vvc_malloc(VVC_MAX_QUERY_STR_SZ);

  sprintf(tmp_query_string, "DELETE FROM VVC WHERE VolID = %d AND BlockName = \'%s\';",
	      vdb->vol_id, vvce->blk_name);

#ifdef VVC_DBG
  vvc_print("Query string: %s\n", tmp_query_string);
#endif
  if (mysql_query(vdb->db_con, tmp_query_string)) {
    fprintf(stderr, "%s\n", mysql_error(vdb->db_con));
    vvc_mfree(tmp_query_string);
    return(-1);
  }
  
  vvc_mfree(tmp_query_string);

  return (0);

}
