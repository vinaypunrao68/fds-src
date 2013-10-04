#include "vvc_private.h"

//#include <my_global.h>
#include <mysql.h>
#include <openssl/lhash.h>

int vvc_db_connect(vvc_vdb_t *vdb);
void vvc_db_disconnect(vvc_vdb_t *vdb);
int vvc_db_entries_load(vvc_vdb_t *vdb);
int vvc_db_entry_load(vvc_vdb_t *vdb, const char *blk_name);
int vvc_db_entry_create(vvc_vdb_t *vdb, vvce_t *vvce);
int vvc_db_entry_update(vvc_vdb_t *vdb, vvce_t *prev_vvce, vvce_t *new_vvce);
int vvc_db_entry_delete(vvc_vdb_t *vdb, vvce_t *vvce);
int vvc_db_vol_delete_entries(vvc_vdb_t *vdb);
