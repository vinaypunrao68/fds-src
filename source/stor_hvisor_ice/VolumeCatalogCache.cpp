/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include "stor_hvisor_ice/VolumeCatalogCache.h"

#include "stor_hvisor_ice/StorHvisorNet.h"

namespace fds {

CatalogCache::CatalogCache() {
}

CatalogCache::~CatalogCache() {
}

Error CatalogCache::Query(fds_uint64_t block_id,
                          ObjectID* oid) {
  Error err(ERR_OK);

  map_rwlock.read_lock();
  try {
    ObjectID& obj = offset_map.at(block_id);
    *oid = obj;
  } catch(const std::out_of_range& oor) {
    err = ERR_CAT_QUERY_FAILED;
  }
  map_rwlock.read_unlock();
  return err;
}

Error CatalogCache::Update(fds_uint64_t block_id, const ObjectID& oid) {
  Error err(ERR_OK);

  /*
   * We already have this mapping. Return duplicate err
   * so that the caller knows not to call DM.
   */
  map_rwlock.write_lock();
  if (offset_map[block_id] == oid) {
    err = ERR_DUPLICATE;
    map_rwlock.write_unlock();
    return err;
  }

  /*
   * Currently, we're always overwriting the existing
   * entry. We should take a flag or something rather
   * that always overwrite.
   */
  offset_map[block_id] = oid;
  map_rwlock.write_unlock();

  return err;
}

void CatalogCache::Clear() {
  map_rwlock.write_lock();
  offset_map.clear();
  map_rwlock.write_unlock();
}

VolumeCatalogCache::VolumeCatalogCache(
    StorHvCtrl *sh_ctrl)
    : parent_sh(sh_ctrl) {
  vcc_log = new fds_log("vcc", "logs");
  created_log = true;
}

VolumeCatalogCache::VolumeCatalogCache(
    StorHvCtrl *sh_ctrl,
    fds_log *parent_log)
    : parent_sh(sh_ctrl),
      vcc_log(parent_log),
      created_log(false) {
}

VolumeCatalogCache::~VolumeCatalogCache() {
  /*
   * Iterate the vol_cache_map to free the
   * catalog pointers.
   */
  map_rwlock.write_lock();
  for (std::unordered_map<fds_uint32_t, CatalogCache*>::iterator it =
           vol_cache_map.begin();
       it != vol_cache_map.end();
       it++) {
    delete it->second;
  }
  vol_cache_map.clear();
  map_rwlock.write_unlock();
  
  /*
   * Delete log if malloc'd one earlier.
   */
  if (created_log == true) {
    delete vcc_log;
  }
}

/*
 * TODO: This will eventually be called when a client is
 * notified of a volume creation by the OM.
 */
Error VolumeCatalogCache::RegVolume(fds_uint64_t vol_uuid) {
  Error err(ERR_OK);

  map_rwlock.write_lock();
  /*
   * TODO: Check if this volume is already
   * registered.
   */
  vol_cache_map[vol_uuid] = new CatalogCache();
  map_rwlock.write_unlock();

  FDS_PLOG(vcc_log) << "Registered new volume " << vol_uuid;

  return err;
}

Error VolumeCatalogCache::Query(fds_uint64_t vol_uuid,
                                fds_uint64_t block_id,
                                fds_uint32_t trans_id,
                                ObjectID *oid) {
  Error err(ERR_OK);
  int   ret_code = 0;

  /*
   * For now, add a new volume cache if we haven't
   * seen this object before. This is hackey, but
   * allows the caller to not need to call RegVolume
   * beforehand.
   */
  map_rwlock.read_lock();
  if (vol_cache_map.count(vol_uuid) == 0) {
    FDS_PLOG(vcc_log) << "Volume " << vol_uuid
                      << " does not exist yet!";
    /*
     * TODO: Potential race here if we release
     * the read lock another thread may register
     * the volume before the write lock is acquired
     * in RegVolume().
     */
    map_rwlock.read_unlock();
    err = RegVolume(vol_uuid);
    if (err != ERR_OK) {
      return err;
    }

    /*
     * Reacquire read lock since we need to access
     * the cache next.
     */
    map_rwlock.read_lock();
  }
  
  /*
   * TODO: Potential race here as we're caching the
   * cache reference. Another thread may delete this
   * volume entry once we release the read lock. We
   * should ensure that the other thread waits until
   * all outstanding references are released before
   * removing. However, we don't delete volumes yet
   * so it's not an issue.
   * We should implement a drain and block using the
   * read-write lock backend.
   */
  CatalogCache *catcache = vol_cache_map[vol_uuid];
  map_rwlock.read_unlock();

  err = catcache->Query(block_id, oid);
  if (!err.ok()) {
    /*
     * Not in the cache. Contact DM.
     */
    FDS_PLOG(vcc_log) << "Cache query for volume " << vol_uuid
                      << " block id " << block_id
                      << " failed. Contacting DM.";

    /*
     * Construct the message to send to DM.
     */
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr =
        new FDS_ProtocolInterface::FDSP_MsgHdrType;
    FDS_ProtocolInterface::FDSP_QueryCatalogTypePtr query_req =
        new FDS_ProtocolInterface::FDSP_QueryCatalogType;
    
    msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_QUERY_CAT_OBJ_REQ;    
    msg_hdr->src_id   = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
    msg_hdr->dst_id   = FDS_ProtocolInterface::FDSP_DATA_MGR;
    msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
    msg_hdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
    msg_hdr->req_cookie = trans_id;

    query_req->volume_offset         = block_id;
    query_req->dm_transaction_id     = 1;
    query_req->dm_operation          =
        FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_OPEN;
    query_req->data_obj_id.hash_high = 0;
    query_req->data_obj_id.hash_low  = 0;

    /*
     * Locate a DM endpoint to try.
     */
    FDS_RPC_EndPoint *endPoint = NULL;
    int node_ids[256]; /* Why 256? Arbitrary. Use better interface. */
    int num_nodes = 0;
    parent_sh->dataPlacementTbl->getDMTNodesForVolume(vol_uuid,
                                                      node_ids,
                                                      &num_nodes);
    fds_uint32_t node_ip;
    int node_state;
    fds_bool_t located_ep = false;
    for (int i = 0; i < num_nodes; i++) {
      err = parent_sh->dataPlacementTbl->getNodeInfo(node_ids[i],
                                                     &node_ip,
                                                     &node_state);
      if (!err.ok()) {
        FDS_PLOG(vcc_log) << "Unable to get node info for node "
                          << node_ids[i];
        return err;
      }

      ret_code = parent_sh->rpcSwitchTbl->Get_RPC_EndPoint(node_ip,
                                                           FDSP_DATA_MGR,
                                                           &endPoint);
      if (ret_code != 0) {
        FDS_PLOG(vcc_log) << "Unable to get RPC endpoint for " << node_ip;
        err = ERR_CAT_QUERY_FAILED;
        return err;
      }

      if (endPoint != NULL) {
        located_ep = true;
        /*
         * We found a valid endpoint. Let's try it
         */
        try {
          endPoint->fdspDPAPI->begin_QueryCatalogObject(msg_hdr, query_req);
          FDS_PLOG(vcc_log) << "Async query request sent to DM " << endPoint
                            << " for volume "<< vol_uuid
                            << " and block id " << block_id;
          
          /*
           * Reset the error to PENDING since we set the message.
           * This lets the caller know to wait for a response.
           */
          err = ERR_PENDING_RESP;
        } catch (...) {
          FDS_PLOG(vcc_log) << "Failed to query DM endpoint " << endPoint
                            << " for volume "<< vol_uuid
                            << " and block id " << block_id;
          err = ERR_CAT_QUERY_FAILED;                    
        }
        break;
      }
    }
    if (located_ep == false) {
      FDS_PLOG(vcc_log) << "Could not locate a valid endpoint to query";
      err = ERR_CAT_QUERY_FAILED;
    }
  }

  return err;
}

/*
 * Update interface for a blob representing a block device.
 */
Error VolumeCatalogCache::Update(fds_uint64_t vol_uuid,
                                 fds_uint64_t block_id,
                                 const ObjectID &oid) {
  Error err(ERR_OK);

  /*
   * For now, add a new volume cache if we haven't
   * seen this object before. This is hackey, but
   * allows the caller to not need to call RegVolume
   * beforehand.
   */
  map_rwlock.read_lock();
  if (vol_cache_map.count(vol_uuid) == 0) {
    /*
     * TODO: Potential race here if we release
     * the read lock another thread may register
     * the volume before the write lock is acquired
     * in RegVolume().
     */
    map_rwlock.read_unlock();
    err = RegVolume(vol_uuid);
    if (err != ERR_OK) {
      return err;
    }

    /*
     * Reacquire read lock since we need to access
     * the cache next.
     */
    map_rwlock.read_lock();
  }

  /*
   * TODO: Potential race here as we're caching the
   * cache reference. Another thread may delete this
   * volume entry once we release the read lock. We
   * should ensure that the other thread waits until
   * all outstanding references are released before
   * removing. However, we don't delete volumes yet
   * so it's not an issue.
   * We should implement a drain and block using the
   * read-write lock backend.
   */
  CatalogCache *catcache = vol_cache_map[vol_uuid];
  map_rwlock.read_unlock();

  err = catcache->Update(block_id, oid);
  /*
   * If the exact entry already existed
   * we can return success.
   */
  if (err == ERR_DUPLICATE) {
    /*
     * Reset the error since it's OK
     * that is already existed.
     */
    FDS_PLOG(vcc_log) << "Cache update successful for volume " << vol_uuid
                      << " block id " << block_id
                      << " was duplicate.";
    err = ERR_OK;
  } else if (err != ERR_OK) {
    FDS_PLOG(vcc_log) << "Cache update for volume " << vol_uuid
                      << " block id " << block_id
                      << " was failed!";
  } else {
    FDS_PLOG(vcc_log) << "Cache update successful for volume " << vol_uuid
                      << " block id " << block_id;
  }

  return err;
}

/*
 * Update interface for a generic blob.
 */

/*
 * Clears the local in-memory cache for a volume.
 */
void VolumeCatalogCache::Clear(fds_uint64_t vol_uuid) {
  
  map_rwlock.read_lock();
  if (vol_cache_map.count(vol_uuid) > 0) {
    /*
     * TODO: Potential race here as we're caching the
     * cache reference. Another thread may delete this
     * volume entry once we release the read lock. We
     * should ensure that the other thread waits until
     * all outstanding references are released before
     * removing. However, we don't delete volumes yet
     * so it's not an issue.
     * We should implement a drain and block using the
     * read-write lock backend.
     */
    CatalogCache *catcache = vol_cache_map[vol_uuid];
    map_rwlock.read_unlock();
    catcache->Clear();
    FDS_PLOG(vcc_log) << "Cleared cache for volume " << vol_uuid;
  } else {
    map_rwlock.read_unlock();
    FDS_PLOG(vcc_log) << "No cache to clear for volume " << vol_uuid;
  }
}

}  // namespace fds
