#include <pthread.h>

#include "include/vvclib.h"
#include "include/tvclib.h"

#include "openssl/lhash.h"

#include "data_mgr_pvt.h"

#include "include/fds_commons.h"

struct __dmgr_vol_cache;

typedef struct __dmgr_transaction{

  fds_uint32_t txn_id;
  fds_uint64_t open_time;
  volid_t vol_id;
  struct __dmgr_vol_cache *vol_info;
  char *blk_name;
  int segment_id;
  fds_doid_t doid;
  int txn_status;
  unsigned int tvc_ref_hint;

} dmgr_txn_t;

unsigned long vol_trans_table_hash(const void *entry);

int vol_trans_table_cmp(const void *arg1, const void *arg2);

typedef struct __dmgr_vol_cache{

  volid_t vol_id;
  _LHASH  *vol_txn_table;
  vvc_vhdl_t vvc_hdl;
  tvc_vhdl_t tvc_hdl;
  pthread_mutex_t vol_txn_table_lock;
  pthread_mutex_t vol_tvc_lock;
  pthread_mutex_t vol_vvc_lock;

} dmgr_vol_cache_t;

unsigned long vol_table_hash(const void *entry);

int vol_table_cmp(const void *arg1, const void *arg2);

extern _LHASH *txn_cache_volume_table; 
extern pthread_mutex_t txn_cache_vol_table_lock;

static char *vol_id_to_tvc_file_name (volid_t vol_id){
  char *tvc_vol_name = (char *)malloc(DMGR_TVC_FILE_NAME_SZ);
  sprintf(tvc_vol_name, "tvc_vol%d", vol_id);
  return (tvc_vol_name);
}

int dmgr_txn_cache_init();

int dmgr_txn_cache_vol_create(volid_t vol_id);

static __inline__ int dmgr_txn_cache_vol_created(volid_t vol_id) {

  dmgr_vol_cache_t dummy_vol_cache;
  dmgr_vol_cache_t *vol_cache;

  dummy_vol_cache.vol_id = vol_id;
  pthread_mutex_lock(&txn_cache_vol_table_lock);
  vol_cache = (dmgr_vol_cache_t *)lh_retrieve(txn_cache_volume_table, &dummy_vol_cache);
  pthread_mutex_unlock(&txn_cache_vol_table_lock);

  if (vol_cache) {
    return (1); //TRUE
  }
  return (0);
}

static __inline__ dmgr_txn_t  *dmgr_txn_create(volid_t vol_id, fds_uint32_t txn_id) {

  dmgr_txn_t *txn;
  dmgr_vol_cache_t *vol_cache, *dummy_vcache;

  txn = (dmgr_txn_t *)malloc (sizeof(dmgr_txn_t));
  memset(txn, 0, sizeof(dmgr_txn_t));
  txn->txn_id = txn_id;
  txn->vol_id = vol_id;
  txn->txn_status = FDS_DMGR_TXN_STATUS_INVALID;
  dummy_vcache = (dmgr_vol_cache_t *)malloc(sizeof(dmgr_vol_cache_t *));
  dummy_vcache->vol_id = vol_id;
  pthread_mutex_lock(&txn_cache_vol_table_lock);
  vol_cache = (dmgr_vol_cache_t *)lh_retrieve(txn_cache_volume_table, dummy_vcache);
  pthread_mutex_unlock(&txn_cache_vol_table_lock);
  free(dummy_vcache);

  if (!vol_cache) {
     free(txn);
     return (0);
  }
  pthread_mutex_lock(&vol_cache->vol_txn_table_lock);
  lh_insert(vol_cache->vol_txn_table, txn);
  txn->vol_info = vol_cache;
  pthread_mutex_unlock(&vol_cache->vol_txn_table_lock);
  txn->tvc_ref_hint = 0;
  return (txn);

}

static __inline__ dmgr_txn_t *dmgr_txn_get(volid_t vol_id, fds_uint32_t txn_id) {

  dmgr_txn_t *txn, *dummy_txn;
  dmgr_vol_cache_t *vol_cache, *dummy_vcache;

  dummy_vcache = (dmgr_vol_cache_t *)malloc(sizeof(dmgr_vol_cache_t *));
  dummy_vcache->vol_id = vol_id;

  pthread_mutex_lock(&txn_cache_vol_table_lock);
  vol_cache = (dmgr_vol_cache_t *)lh_retrieve(txn_cache_volume_table, dummy_vcache);
  pthread_mutex_unlock(&txn_cache_vol_table_lock);
  free(dummy_vcache);
  if (!vol_cache) {
     return (0);
  }
  pthread_mutex_unlock(&txn_cache_vol_table_lock);
  dummy_txn = (dmgr_txn_t *)malloc(sizeof(dmgr_txn_t));
  dummy_txn->txn_id = txn_id;
  pthread_mutex_lock(&vol_cache->vol_txn_table_lock);
  txn = (dmgr_txn_t *)lh_retrieve(vol_cache->vol_txn_table, dummy_txn);
  pthread_mutex_unlock(&vol_cache->vol_txn_table_lock);
  free(dummy_txn);
  return (txn);

}

static __inline__ int dmgr_fill_txn_info(dmgr_txn_t *txn, dm_open_txn_req_t *ot_req) {

  txn->blk_name = (char *)malloc(32);
  sprintf(txn->blk_name, "%llu", ot_req->vvc_blk_id);
  txn->segment_id = 0;
  memcpy(&txn->doid, &ot_req->vvc_obj_id, sizeof(fds_doid_t));
  txn->open_time = ot_req->vvc_update_time;
  return (0);
}

#define update_txn_status(txn, status)   txn->txn_status = status

int dmgr_txn_open(dmgr_txn_t *txn);

int dmgr_txn_commit(dmgr_txn_t *txn);

int dmgr_txn_cancel(dmgr_txn_t *txn);

static __inline__ int dmgr_txn_destroy(dmgr_txn_t *txn) {

  dmgr_vol_cache_t *vol_cache;

  vol_cache = txn->vol_info;

  pthread_mutex_lock(&vol_cache->vol_txn_table_lock);
  if (lh_delete(vol_cache->vol_txn_table, txn) == 0) {
    return (-1);
  }
  pthread_mutex_unlock(&vol_cache->vol_txn_table_lock);
  free(txn);
  return (0);

}
