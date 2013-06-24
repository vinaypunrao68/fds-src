// API declarations 
#include "fds_commons.h"

typedef void *tvc_vhdl_t;

typedef struct __tvcentry {
  
  uint64_t  timestamp;
  char *blk_name;
  int segment_id;
  doid_t doid;
  
} tvce_t;

tvc_vhdl_t tvc_vol_create(volid_t vol_id, const char *db_name, const char *file_path, unsigned int max_file_sz);

tvc_vhdl_t tvc_vol_load(volid_t vol_id, const char *db_name, const char *file_path);

int tvc_entry_append(tvc_vhdl_t vhdl, uint64_t timestamp, const char *blk_name, int segment_id, const doid_t doid);

int tvc_mark_checkpoint(tvc_vhdl_t vhdl, uint64_t timestamp, volid_t tpc_vol_id);

int tvc_entries_get(tvc_vhdl_t vhdl, uint64_t start_time, int *n_entries, tvce_t *tvc_entries, int *end_of_log);
