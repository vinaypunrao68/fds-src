/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include "stor_hvisor_ice/VolumeCatalogCache.h"

#include "stor_hvisor_ice/StorHvisorNet.h"

extern StorHvCtrl *storHvisor;

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
    fds_volid_t _vol_id,
    StorHvCtrl *sh_ctrl)
    : vol_id(_vol_id),
      parent_sh(sh_ctrl) {
  vcc_log = new fds_log("vcc", "logs");
  created_log = true;
}

VolumeCatalogCache::VolumeCatalogCache(
    fds_volid_t _vol_id,
    StorHvCtrl *sh_ctrl,
    fds_log *parent_log)
    : vol_id(_vol_id),
      parent_sh(sh_ctrl),
      vcc_log(parent_log),
      created_log(false) {
}

VolumeCatalogCache::~VolumeCatalogCache() {
  /*
   * Delete log if malloc'd one earlier.
   */
  if (created_log == true) {
    delete vcc_log;
  }
}

Error VolumeCatalogCache::Query(fds_uint64_t block_id,
                                fds_uint32_t trans_id,
                                ObjectID *oid) {
  Error err(ERR_OK);
  int   ret_code = 0;

  err = cat_cache.Query(block_id, oid);
  if (!err.ok()) {
    /*
     * Not in the cache. Contact DM.
     */
    FDS_PLOG(vcc_log) << "VolumeCatalogCache - Cache query for volume "
                      << vol_id << " block id " << block_id
                      << " failed. Contacting DM.";

    /*
     * Construct the message to send to DM.
     */
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr =
        new FDS_ProtocolInterface::FDSP_MsgHdrType;
    FDS_ProtocolInterface::FDSP_QueryCatalogTypePtr query_req =
        new FDS_ProtocolInterface::FDSP_QueryCatalogType;

    msg_hdr->glob_volume_id = vol_id;
    msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_QUERY_CAT_OBJ_REQ;
    msg_hdr->src_id   = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
    msg_hdr->src_node_name = storHvisor->my_node_name;
    msg_hdr->dst_id   = FDS_ProtocolInterface::FDSP_DATA_MGR;
    msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
    msg_hdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
    msg_hdr->req_cookie = trans_id;

    query_req->blob_name         = std::to_string(block_id);
    query_req->dm_transaction_id     = 1;
    query_req->dm_operation          =
        FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_OPEN;
    query_req->obj_list.clear();
    query_req->meta_list.clear();

    /*
     * Locate a DM endpoint to try.
     */
    FDS_RPC_EndPoint *endPoint = NULL;
    int node_ids[8];
    int num_nodes = 8;
    parent_sh->dataPlacementTbl->getDMTNodesForVolume(vol_id,
                                                      node_ids,
                                                      &num_nodes);
    fds_uint32_t node_ip;
    fds_uint32_t node_port;
    int node_state;
    fds_bool_t located_ep = false;
    for (int i = 0; i < num_nodes; i++) {
      err = parent_sh->dataPlacementTbl->getNodeInfo(node_ids[i],
                                                     &node_ip,
                                                     &node_port,
                                                     &node_state);
      if (!err.ok()) {
        FDS_PLOG(vcc_log) << "VolumeCatalogCache - "
                          << "Unable to get node info for node "
                          << node_ids[i];
        return err;
      }

      ret_code = parent_sh->rpcSwitchTbl->Get_RPC_EndPoint(node_ip,
                                                           FDSP_DATA_MGR,
                                                           &endPoint);
      if (ret_code != 0) {
        FDS_PLOG(vcc_log) << "VolumeCatalogCache - "
                          << "Unable to get RPC endpoint for "
                          << node_ip;
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
          FDS_PLOG(vcc_log) << " VolumeCatalogCache - "
                            << "Async query request sent to DM "
                            << endPoint << " for volume " << vol_id
                            << " and block id " << block_id;
          /*
           * Reset the error to PENDING since we set the message.
           * This lets the caller know to wait for a response.
           */
          err = ERR_PENDING_RESP;
        } catch(...) {
          FDS_PLOG(vcc_log) << "VolumeCatalogCache - "
                            << "Failed to query DM endpoint "
                            << endPoint << " for volume "<< vol_id
                            << " and block id " << block_id;
          err = ERR_CAT_QUERY_FAILED;
        }
        break;
      }
    }
    if (located_ep == false) {
      FDS_PLOG(vcc_log) << " VolumeCatalogCache - "
                        << "Could not locate a valid endpoint to query";
      err = ERR_CAT_QUERY_FAILED;
    }
  }

  return err;
}

/*
 * Update interface for a blob representing a block device.
 */
Error VolumeCatalogCache::Update(fds_uint64_t block_id,
                                 const ObjectID &oid) {
  Error err(ERR_OK);

  err = cat_cache.Update(block_id, oid);
  /*
   * If the exact entry already existed
   * we can return success.
   */
  if (err == ERR_DUPLICATE) {
    /*
     * Reset the error since it's OK
     * that is already existed.
     */
    FDS_PLOG(vcc_log) << "VolumeCatalogCache - "
                      << "Cache update successful for volume "
                      << vol_id << " block id " << block_id
                      << " was duplicate.";
    err = ERR_OK;
  } else if (err != ERR_OK) {
    FDS_PLOG(vcc_log) << "VolumeCatalogCache - "
                      << "Cache update for volume " << vol_id
                      << " block id " << block_id << " was failed!";
  } else {
    FDS_PLOG(vcc_log) << "VolumeCatalogCache - "
                      << "Cache update successful for volume "
                      << vol_id << " block id " << block_id;
  }

  return err;
}

/*
 * Update interface for a generic blob.
 */

/*
 * Clears the local in-memory cache for a volume.
 */
void VolumeCatalogCache::Clear() {
    cat_cache.Clear();
    FDS_PLOG(vcc_log) << "VolumeCatalogCache - Cleared cache for volume "
                      << vol_id;
}

}  // namespace fds
