/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include "stor_hvisor_ice/VolumeCatalogCache.h"

namespace fds {

CatalogCache::CatalogCache() {
  
}

CatalogCache::~CatalogCache() {
}

Error CatalogCache::Query(fds_uint64_t block_id,
                          ObjectID* oid) {
  Error err(ERR_OK);

  try {
    ObjectID& obj = offset_map.at(block_id);
    *oid = obj;
  } catch (const std::out_of_range& oor) {
    err = ERR_CAT_QUERY_FAILED;
  }

  return err;
}

Error CatalogCache::Update(fds_uint64_t block_id, const ObjectID& oid) {
  Error err(ERR_OK);

  /*
   * We already have this mapping. Return duplicate err
   * so that the caller knows not to call DM.
   */
  if (offset_map[block_id] == oid) {
    err = ERR_DUPLICATE;
    return err;
  }

  /*
   * Currently, we're always overwriting the existing
   * entry. We should take a flag or something rather
   * that always overwrite.
   */
  offset_map[block_id] = oid;

  return err;
}

/*
 * TODO: Change this interface once we get a generic network
 * API. This current interface can ONLY talk to ONE DM!
 */
VolumeCatalogCache::VolumeCatalogCache(
    FDS_ProtocolInterface::FDSP_DataPathReqPrx& fdspDPAPI_arg)
    : fdspDPAPI(fdspDPAPI_arg) {  
}

VolumeCatalogCache::~VolumeCatalogCache() {
}

/*
 * TODO: This will eventually be called when a client is
 * notified of a volume creation by the OM.
 */
Error VolumeCatalogCache::RegVolume(fds_uint64_t vol_uuid) {
  Error err(ERR_OK);

  vol_cache_map[vol_uuid] = CatalogCache();

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
  if (vol_cache_map.count(vol_uuid) == 0) {
    err = RegVolume(vol_uuid);

    if (err != ERR_OK) {
      return err;
    }
  }

  CatalogCache& catcache = vol_cache_map[vol_uuid];

  err = catcache.Update(block_id, oid);
  /*
   * If the exact entry already existed
   * we can return success.
   */
  if (err == ERR_DUPLICATE) {
    err = ERR_OK;
    return err;
  } else if (err != ERR_OK) {
    return err;
  }

  /*
   * We added a new entry to the catalog cache
   * so we need to notify the DM.
   *
   * TODO: Create a single request for now. We should have a shared
   * of requests that go as thread specific pointers for each thread
   * in the thread pool.
   */
  FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr = new FDS_ProtocolInterface::FDSP_MsgHdrType;
  FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr update_req = new FDS_ProtocolInterface::FDSP_UpdateCatalogType;
  
  update_req->volume_offset         = block_id;
  update_req->dm_transaction_id     = 1;
  update_req->dm_operation          = FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_OPEN;
  update_req->data_obj_id.hash_high = oid.GetHigh();
  update_req->data_obj_id.hash_low  = oid.GetLow();
  
  fdspDPAPI->UpdateCatalogObject(msg_hdr, update_req);
  
  return err;
}

/*
 * Update interface for a generic blob.
 */

}  // namespace fds
