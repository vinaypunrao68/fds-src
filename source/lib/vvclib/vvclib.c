#include <string.h>

// #define MAX_BLK_NAME_SZ           255
#include "fds_commons.h"
#include "vvc_private.h"
#include "vvclib.h"

unsigned long vvce_hash(const void *entry) {
  int i;
  int byte_offset = 0;
  unsigned long hash = 0;
  const vvce_t *vvce = (vvce_t *)entry;
  int len = strlen(vvce->blk_name);

  // return (lh_strhash(vvce->blk_name));

  for (i = 0; i < len; i++) {
    char this_char = vvce->blk_name[i];
    byte_offset = (byte_offset+1) % 4;
    hash = hash ^ (this_char << byte_offset);
  }
  return (hash);
}

int vvce_cmp(const void *arg1, const void *arg2) {
  const vvce_t *vvce1 = (vvce_t *)arg1;
  const vvce_t *vvce2 = (vvce_t *)arg2;
  return (strcmp(vvce1->blk_name, vvce2->blk_name));
}

vvc_vhdl_t vvc_vol_create(volid_t vol_id, const char *db_name, int max_blocks) {

  vvc_vdb_t *vdb = (vvc_vdb_t *)vvc_malloc(sizeof(vvc_vdb_t));
  memset(vdb, 0, sizeof(vvc_vdb_t));
  vdb->vol_id = vol_id;
  if (db_name != NULL) {
    #ifdef LIB_KERNEL
    return (0);
    #endif
    vdb->db_name = (char *)vvc_malloc(strlen(db_name) + 1);
    strcpy(vdb->db_name, db_name);
    if (vvc_db_connect(vdb)){
      return (0);
    }    
  }
  vdb->vvc_table = lh_new(vvce_hash, vvce_cmp);
  vdb->max_num_blks = max_blocks;
  return (vvc_vhdl_t)vdb;

}

#ifndef LIB_KERNEL
// This is used to load an existing volume from DB. In-memory DB clients should always call vol_create.
vvc_vhdl_t vvc_vol_load(volid_t vol_id, const char *db_name) {

  vvc_vdb_t *vdb = (vvc_vdb_t *)vvc_malloc(sizeof(vvc_vdb_t));
  memset(vdb, 0, sizeof(vvc_vdb_t));
  vdb->vol_id = vol_id;
  if (db_name == NULL) {
    return (0);
  }
  vdb->db_name = (char *)vvc_malloc(strlen(db_name) + 1);
  strcpy(vdb->db_name, db_name);
  if (vvc_db_connect(vdb)){
    return (0);
  }

  vdb->vvc_table = lh_new(vvce_hash, vvce_cmp);
  vdb->max_num_blks = 0;
  if (vvc_db_entries_load(vdb)) {
    return (0);
  }
  return (vvc_vhdl_t)vdb;

  return (0);
}
#endif

int vvc_entry_create(vvc_vhdl_t vhdl, const char *blk_name, int num_segments, const doid_t **doid_list) {

  vvc_vdb_t *vdb = (vvc_vdb_t *)vhdl;
  vvce_t *prev_entry;
  vvce_t *vvce;
  int rc;

  vvce = vvce_create(blk_name, num_segments, doid_list);

  prev_entry = lh_retrieve(vdb->vvc_table, vvce);
  if (prev_entry) {
    vvce_destroy(vvce);
    return (-EEXISTS);
  }
  lh_insert(vdb->vvc_table, vvce);
  if (rc = vvc_db_entry_create(vdb, vvce)) {
    return(rc);
  }
  
  return (0);

}

int vvc_entry_get(vvc_vhdl_t vhdl, const char *blk_name, int *num_segments, doid_t **doid_list) {

  vvc_vdb_t *vdb = (vvc_vdb_t *)vhdl;
  vvce_t *tmp_vvce, *vvce;

  tmp_vvce = vvce_create(blk_name, 0, 0);
  vvce = lh_retrieve(vdb->vvc_table, tmp_vvce);
  vvce_destroy(tmp_vvce);

  if (vvce) {

    *num_segments = vvce->num_segments;
    *doid_list = (doid_t *)vvc_malloc(vvce->num_segments * sizeof(doid_t));
    memcpy(*doid_list, vvce->doid_list, vvce->num_segments * sizeof(doid_t));
    return (0);

  } else {

    *num_segments = 0;
    *doid_list = 0;
    return (-ENOENTRY);

  }

}

int vvc_entry_update(vvc_vhdl_t vhdl, const char *blk_name, int num_segments, const doid_t **doid_list) {

  vvc_vdb_t *vdb = (vvc_vdb_t *)vhdl;
  vvce_t *prev_entry;
  vvce_t *vvce;
  int rc;

  vvce = vvce_create(blk_name, num_segments, doid_list);
  prev_entry = lh_insert(vdb->vvc_table, vvce);

  rc = vvc_db_entry_update(vdb, prev_entry, vvce);

  if (prev_entry) {
    vvce_destroy(prev_entry);
  }

  return (rc);

}

int vvc_entry_add(vvc_vhdl_t vhdl, const char *blk_name, int segment_num, const doid_t *doid);

int vvc_entry_remove(vvc_vhdl_t vhdl, const char *blk_name, int segment_num);

int vvc_entry_delete(vvc_vhdl_t vhdl, const char *blk_name) {

  vvc_vdb_t *vdb = (vvc_vdb_t *)vhdl;
  vvce_t *tmp_vvce, *vvce;
  int rc;

  tmp_vvce = vvce_create(blk_name, 0, 0);
  vvce = lh_delete(vdb->vvc_table, tmp_vvce);
  if (vvce) {
    rc = vvc_db_entry_delete(vdb, vvce);
    vvce_destroy(vvce);
  }
  vvce_destroy(tmp_vvce);
  return (rc);

}
