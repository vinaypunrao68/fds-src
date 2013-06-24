#include <openssl/lhash.h>
#include <my_global.h>
#include <mysql.h>

#include "fds_commons.h"

#ifdef LIB_KERNEL

#define vvc_malloc(size) kmalloc(size, GFP_KERNEL)
#define vvc_mfree(ptr)  kfree(ptr)
#define vvc_print printk

#else

#define vvc_malloc(size) malloc(size)
#define vvc_mfree(ptr) free(ptr)
#define vvc_print printf

#endif

typedef struct __vvc_entry {

  char       *blk_name;
  int        num_segments;
  int        doid_list_size;
  doid_t     *doid_list;

} vvce_t;

static __inline__ vvce_t *vvce_create(const char *blk_name, const int num_segments, const doid_t **doid_list) {

  int i;

  vvce_t *vvce = (vvce_t *)vvc_malloc(sizeof(vvce_t));

  memset(vvce, 0, sizeof(vvce_t));
  vvce->blk_name = (char *)vvc_malloc(strlen(blk_name) + 1);
  strcpy(vvce->blk_name, blk_name);
  vvce->num_segments = num_segments;
  vvce->doid_list_size = 2*num_segments;
  vvce->doid_list =  (doid_t *)vvc_malloc(vvce->doid_list_size * sizeof(doid_t));
  memset(vvce->doid_list, 0, vvce->doid_list_size * sizeof(doid_t));
  for (i = 0; i < num_segments; i++) {
    memcpy(&vvce->doid_list[i], doid_list[i], sizeof(doid_t));
  }

  return(vvce);

}

static __inline__ void vvce_destroy(vvce_t *vvce) {

  if (vvce) {
    if (vvce->blk_name) {
      vvc_mfree(vvce->blk_name);
    }
    if (vvce->doid_list) {
      vvc_mfree(vvce->doid_list);
    }
    vvc_mfree(vvce);
  }
  return;
}

typedef struct __vvc_vdb {

  volid_t    vol_id;
  char       *db_name;
  MYSQL      *db_con;
  int        num_blks;
  int        max_num_blks;
  _LHASH     *vvc_table; // a hash table containing vvc_entry_t structures

} vvc_vdb_t;

int vvc_db_connect(vvc_vdb_t *vdb);
int vvc_db_entries_load(vvc_vdb_t *vdb);
int vvc_db_entry_load(vvc_vdb_t *vdb, const char *blk_name);
int vvc_db_entry_create(vvc_vdb_t *vdb, vvce_t *vvce);
int vvc_db_entry_update(vvc_vdb_t *vdb, vvce_t *prev_vvce, vvce_t *new_vvce);
int vvc_db_entry_delete(vvc_vdb_t *vdb, vvce_t *vvce);
