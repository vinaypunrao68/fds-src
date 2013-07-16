// API declarations 
#define MAX_BLK_NAME_SZ           255
#include "fds_commons.h"

typedef void *vvc_vhdl_t;

vvc_vhdl_t vvc_vol_create(volid_t vol_id, const char *db_name, int max_blocks);

// This is used to load an existing volume from DB. In-memory DB clients should always call vol_create.
vvc_vhdl_t vvc_vol_load(volid_t vol_id, const char *db_name);

int vvc_entry_create(vvc_vhdl_t vhdl, const char *blk_name, int num_segments, const doid_t **doid_list);

int vvc_entry_get(vvc_vhdl_t vhdl, const char *blk_name, int *num_segments, doid_t **doid_list);

int vvc_entry_update(vvc_vhdl_t vhdl, const char *blk_name, int num_segments, const doid_t **doid_list);

int vvc_entry_add(vvc_vhdl_t vhdl, const char *blk_name, int segment_num, const doid_t *doid);

int vvc_entry_remove(vvc_vhdl_t vhdl, const char *blk_name, int segment_num);

int vvc_entry_delete(vvc_vhdl_t vhdl, const char *blk_name);

enum {

  SUCCESS,
  EEXISTS,
  ENOENTRY

};
