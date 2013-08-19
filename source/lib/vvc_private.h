#ifndef LIB_KERNEL
#include <my_global.h>
#include <mysql.h>
#include <openssl/lhash.h>
#else
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include "lhash.h"
#endif

// #include "fds_commons.h"
#include "include/fds_commons_rm.h"

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


vvce_t *vvce_create(const char *blk_name, const int num_segments, const doid_t **doid_list);

void vvce_destroy(vvce_t *vvce);

typedef struct __vvc_vdb {

  volid_t    vol_id;
  char       *db_name;
#ifndef LIB_KERNEL
  MYSQL      *db_con;
#else
  void       *db_con;
#endif
  int        num_blks;
  int        max_num_blks;
  _LHASH     *vvc_table; // a hash table containing vvc_entry_t structures

} vvc_vdb_t;

