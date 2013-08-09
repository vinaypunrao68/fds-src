#include "data_mgr_transactions.h"

int dmgr_txn_cache_init() {
  
  txn_cache_volume_table = lh_new(vol_table_hash, vol_table_cmp);
  pthread_mutex_init(&txn_cache_vol_table_lock, NULL);
  return (0);

}

unsigned long vol_trans_table_hash(const void *entry) {
  const dmgr_txn_t *txn = (dmgr_txn_t *)entry;
  return (txn->txn_id);
}

int vol_trans_table_cmp(const void *arg1, const void *arg2) {
  const dmgr_txn_t *txn1 = (dmgr_txn_t *)arg1;
  const dmgr_txn_t *txn2 = (dmgr_txn_t *)arg2;
  if (txn1->txn_id == txn2->txn_id) {
    return (0);
  }
  if (txn1->txn_id < txn2->txn_id) {
    return (-1);
  }
  return (1);  
}

unsigned long vol_table_hash(const void *entry) {
  const dmgr_vol_cache_t *vcache = (dmgr_vol_cache_t *)entry;
  return (vcache->vol_id);
}

int vol_table_cmp(const void *arg1, const void *arg2) {
  const dmgr_vol_cache_t *vcache1 = (dmgr_vol_cache_t *)arg1;
  const dmgr_vol_cache_t *vcache2 = (dmgr_vol_cache_t *)arg2;
  if (vcache1->vol_id == vcache2->vol_id) {
    return (0);
  }
  if (vcache1->vol_id < vcache2->vol_id) {
    return (-1);
  }
  return (1);  
}

int dmgr_txn_cache_vol_create(volid_t vol_id) {
  
  dmgr_vol_cache_t *vol_cache;
  char *tvc_file_name;

  vol_cache = (dmgr_vol_cache_t *)malloc(sizeof(dmgr_vol_cache_t));

  vol_cache->vol_id = vol_id;
  vol_cache->vol_txn_table = lh_new(vol_trans_table_hash, vol_trans_table_cmp);
  pthread_mutex_init(&vol_cache->vol_txn_table_lock, NULL);
  pthread_mutex_init(&vol_cache->vol_vvc_lock, NULL);
  pthread_mutex_init(&vol_cache->vol_tvc_lock, NULL);
  vol_cache->vvc_hdl = vvc_vol_load(vol_id, "FDS_SP_DB");
  if (vol_cache->vvc_hdl == NULL) {
    vol_cache->vvc_hdl = vvc_vol_create(vol_id, "FDS_SP_DB", 0);
    if (vol_cache->vvc_hdl == NULL) {
      return (-1);
    }
  }
  tvc_file_name = vol_id_to_tvc_file_name(vol_id);
  vol_cache->tvc_hdl = tvc_vol_load(vol_id, "FDS_SP_DB", tvc_file_name);
  if (vol_cache->tvc_hdl == NULL) {
    vol_cache->tvc_hdl = tvc_vol_create(vol_id, "FDS_SP_DB", tvc_file_name, DMGR_MAX_TVC_FILE_SZ);
    if (vol_cache->tvc_hdl == NULL) {
      free(tvc_file_name);
      free(vol_cache);
      return (-1);
    }
  }
  free(tvc_file_name);
  pthread_mutex_lock(&txn_cache_vol_table_lock);
  lh_insert(txn_cache_volume_table, vol_cache);
  pthread_mutex_unlock(&txn_cache_vol_table_lock);

  return(0);
}

int dmgr_txn_open(dmgr_txn_t *txn) {

    dmgr_vol_cache_t *vol_cache;
    unsigned int ref_hint;

    vol_cache = txn->vol_info;
    pthread_mutex_lock(&vol_cache->vol_tvc_lock);
    if (tvc_entry_append(vol_cache->tvc_hdl, txn->txn_id, txn->open_time, txn->blk_name, 0, (const unsigned char *)&(txn->doid.bytes[0]), &ref_hint) < 0) {
      pthread_mutex_unlock(&vol_cache->vol_tvc_lock);
      return (-1);
    }
    txn->tvc_ref_hint = ref_hint;
    txn->txn_status = FDS_DMGR_TXN_STATUS_OPEN;
    pthread_mutex_unlock(&vol_cache->vol_tvc_lock);
    return (0);
}

int dmgr_txn_commit(dmgr_txn_t *txn) {

    dmgr_vol_cache_t *vol_cache;
    vol_cache = txn->vol_info;
    int n_segments, rc;
    doid_t *doid_list;
    doid_t *p_doid;

    // Step 1: Update VVC
    pthread_mutex_lock(&vol_cache->vol_vvc_lock);
    vvc_entry_get(vol_cache->vvc_hdl, txn->blk_name, &n_segments, &doid_list);
    p_doid = (doid_t *)&(txn->doid.bytes[0]); 
    if (n_segments <= 0) {
      rc = vvc_entry_create(vol_cache->vvc_hdl, txn->blk_name, 1, (const doid_t **)&p_doid);
    } else {
      free(doid_list);
      rc = vvc_entry_update(vol_cache->vvc_hdl, txn->blk_name, 1, (const doid_t **)&p_doid);
    }
    pthread_mutex_unlock(&vol_cache->vol_vvc_lock);
    if (rc < 0) {
      return (-1);
    }

    // Step 2: If it's a modify, send Association Table Update to SM and keep the transaction around.

    // Step 3: Mark txn as commited in TVC
    pthread_mutex_lock(&vol_cache->vol_tvc_lock);
    rc = tvc_entry_status_update(vol_cache->tvc_hdl, txn->txn_id, txn->tvc_ref_hint, FDS_DMGR_TXN_STATUS_COMMITED);
    pthread_mutex_unlock(&vol_cache->vol_tvc_lock);
    if (rc < 0) {
      return (-1);
    }
    txn->txn_status = FDS_DMGR_TXN_STATUS_COMMITED;
    return (0);
}

int dmgr_txn_cancel(dmgr_txn_t *txn) {

    dmgr_vol_cache_t *vol_cache;
    vol_cache = txn->vol_info;
    int rc;

    // Step 2: Mark txn as commited in TVC
    pthread_mutex_lock(&vol_cache->vol_tvc_lock);
    rc = tvc_entry_status_update(vol_cache->tvc_hdl, txn->txn_id, txn->tvc_ref_hint, FDS_DMGR_TXN_STATUS_CANCELED);
    pthread_mutex_unlock(&vol_cache->vol_tvc_lock);
    if (rc < 0) {
      return (-1);
    }
    txn->txn_status = FDS_DMGR_TXN_STATUS_CANCELED;
    return (0);
}
