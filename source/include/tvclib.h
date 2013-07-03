// API declarations 
#include "fds_commons.h"

typedef void *tvc_vhdl_t;

typedef struct __tvcentry {
  
  fds_uint64_t  timestamp;
  char *blk_name;
  int segment_id;
  doid_t doid;
  
} tvce_t;

tvc_vhdl_t tvc_vol_create(volid_t vol_id, const char *db_name, const char *file_path, unsigned int max_file_sz);

tvc_vhdl_t tvc_vol_load(volid_t vol_id, const char *db_name, const char *file_path);

int tvc_entry_append(tvc_vhdl_t vhdl, fds_uint32_t txn_id, fds_uint64_t timestamp, const char *blk_name, int segment_id, const doid_t doid, unsigned int *entry_ref_hint);

int tvc_entry_status_update(tvc_vhdl_t vhdl, fds_uint32_t txn_id, int64_t entry_ref_hint, unsigned char status);

int tvc_mark_checkpoint(tvc_vhdl_t vhdl, fds_uint64_t timestamp, volid_t tpc_vol_id);

int tvc_entries_get(tvc_vhdl_t vhdl, fds_uint64_t start_time, int *n_entries, tvce_t *tvc_entries, int *end_of_log);
