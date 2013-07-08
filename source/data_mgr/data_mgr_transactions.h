struct __dmgr_vol_cache;

typedef struct __dmgr_transaction{

  fds_uint32_t txn_id;
  fds_uint64_t open_time;
  volid_t vol_id;
  struct __dmgr_vol_cache *vol_info;
  char *blk_name;
  int segment_id;
  doid_t doid;
  int txn_status;
  unsigned int tvc_ref_hint;

} dmgr_txn_t;

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


typedef struct __dmgr_vol_cache{

  volid_t vol_id;
  _LHASH  *vol_txn_table;
  vvc_vhdl_t vvc_hdl;
  tvc_vhdl_t tvc_hdl;
  pthread_mutex_t vol_txn_table_lock;
  pthread_mutex_t vol_tvc_lock;
  pthread_mutex_t vol_vvc_lock;

} dmgr_vol_cache_t;

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


extern _LHASH *txn_cache_volume_table; 
extern pthread_mutex_t txn_cache_vol_table_lock;

static __inline__ char *vol_id_to_tvc_file_name (vol_id){
  char *tvc_vol_name = (char *)malloc(DMGR_TVC_FILE_NAME_SZ);
  sprintf(tvc_vol_name, "tvc_vol%d", vol_id);
  return (tvc_vol_name);
}

static __inline__ int dmgr_txn_cache_init() {
  
  txn_cache_volume_table = lh_new(vol_table_hash, vol_table_cmp);
  pthread_mutex_init(&txn_cache_vol_table_lock, NULL);
  return (0);

}

static __inline__ int dmgr_txn_cache_vol_create(volid_t vol_id) {

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
  vol_cache = lh_retrieve(txn_cache_volume_table, dummy_vcache);
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
  vol_cache = lh_retrieve(txn_cache_volume_table, dummy_vcache);
  pthread_mutex_unlock(&txn_cache_vol_table_lock);
  free(dummy_vcache);
  if (!vol_cache) {
     return (0);
  }
  pthread_mutex_unlock(&txn_cache_vol_table_lock);
  dummy_txn = (dmgr_txn_t *)malloc(sizeof(dmgr_txn_t));
  dummy_txn->txn_id = txn_id;
  pthread_mutex_lock(&vol_cache->vol_txn_table_lock);
  txn = lh_retrieve(vol_cache->vol_txn_table, dummy_txn);
  pthread_mutex_unlock(&vol_cache->vol_txn_table_lock);
  free(dummy_txn);
  return (txn);

}

static __inline__ int dmgr_fill_txn_info(dmgr_txn_t *txn, dm_open_txn_req_t *ot_req) {

  txn->blk_name = (char *)malloc(32);
  sprintf(txn->blk_name, "%llu", ot_req->vvc_blk_id);
  txn->segment_id = 0;
  memcpy(txn->doid, ot_req->vvc_obj_id, sizeof(doid_t));
  txn->open_time = ot_req->vvc_update_time;
  return (0);
}

#define update_txn_status(txn, status)   txn->txn_status = status

static __inline__ int dmgr_txn_open(dmgr_txn_t *txn) {

    dmgr_vol_cache_t *vol_cache;
    unsigned int ref_hint;

    vol_cache = txn->vol_info;
    pthread_mutex_lock(&vol_cache->vol_tvc_lock);
    if (tvc_entry_append(vol_cache->tvc_hdl, txn->txn_id, txn->open_time, txn->blk_name, 0, txn->doid, &ref_hint) < 0) {
      pthread_mutex_unlock(&vol_cache->vol_tvc_lock);
      return (-1);
    }
    txn->tvc_ref_hint = ref_hint;
    txn->txn_status = FDS_DMGR_TXN_STATUS_OPEN;
    pthread_mutex_unlock(&vol_cache->vol_tvc_lock);
    return (0);
}


static __inline__ int dmgr_txn_commit(dmgr_txn_t *txn) {

    dmgr_vol_cache_t *vol_cache;
    vol_cache = txn->vol_info;
    int n_segments, rc;
    doid_t *doid_list;
    doid_t *p_doid;

    // Step 1: Update VVC
    pthread_mutex_lock(&vol_cache->vol_vvc_lock);
    vvc_entry_get(vol_cache->vvc_hdl, txn->blk_name, &n_segments, &doid_list);
    p_doid = &txn->doid; 
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

static __inline__ int dmgr_txn_cancel(dmgr_txn_t *txn) {

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
