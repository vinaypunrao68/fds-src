/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <string>

#include <VolumeCatalogCache.h>
#include <StorHvisorNet.h>

extern StorHvCtrl *storHvisor;

namespace fds {

CatalogCache::CatalogCache(const std::string &name)
        : blobName(name),
          blobSize(0) {
}

CatalogCache::~CatalogCache() {
}

Error CatalogCache::Query(fds_uint64_t block_id,
                          ObjectID* oid) {
    Error err(ERR_OK);

    map_rwlock.read_lock();
    try {
        ObjectID& obj = (offset_map.at(block_id)).first;
        *oid = obj;
    } catch(const std::out_of_range& oor) {
        err = ERR_CAT_QUERY_FAILED;
    }
    map_rwlock.read_unlock();
    return err;
}


Error CatalogCache::Update(fds_uint64_t blobOffset,
                           fds_uint32_t objectLen,
                           const ObjectID& oid,
                           fds_bool_t lastBuf) {
    Error err(ERR_OK);

    map_rwlock.write_lock();
    if (offset_map.count(blobOffset) != 0) {
        if (oid != NullObjectID) {
            fds_verify(blobSize >= 0);
        }

        // We already have this mapping. Return duplicate err
        // so that the caller knows not to call DM.
        if ((offset_map[blobOffset]).first == oid) {
            fds_verify((offset_map[blobOffset]).second == objectLen);
            err = ERR_DUPLICATE;
            map_rwlock.write_unlock();
            return err;
        }

        blobSize -= (offset_map[blobOffset]).second;
    }

    /*
     * Currently, we're always overwriting the existing
     * entry. We should take a flag or something rather
     * that always overwrite.
     */
    offset_map[blobOffset] = ObjectDesc(oid, objectLen);
    blobSize += objectLen;

    // If this is the lastBuf, remove any other old offsets
    // that are beyond this offset
    if (lastBuf == true) {
        OffsetMap::iterator it = offset_map.begin();
        while (it != offset_map.end()) {
            if ((*it).first > blobOffset) {
                blobSize -= (*it).second.second;
                it = offset_map.erase(it);
            } else {
                it++;
            }
        }
    }

    LOGDEBUG << "Updated cached blob " << blobName
             << " to size " << blobSize << " by updating "
             << blobOffset << " with size " << objectLen;
    map_rwlock.write_unlock();

    return err;
}

fds_uint64_t CatalogCache::getBlobSize() {
    fds_uint64_t size;
    map_rwlock.read_lock();
    size = blobSize;
    map_rwlock.read_unlock();
    return size;
}

std::string CatalogCache::getBlobEtag() {
    std::string etag;
    map_rwlock.read_lock();
    etag = blobEtag;
    map_rwlock.read_unlock();
    return etag;
}

void CatalogCache::setBlobEtag(const std::string &etag) {
    map_rwlock.read_lock();
    blobEtag = etag;
    map_rwlock.read_unlock();
    // It it's being set, it should be to
    // a valid value
    // TODO(Andrew): Remove this when etag moves
    // to updateMetadata path.
    // fds_verify(blobEtag.size() == 32);
}

void CatalogCache::Clear() {
    map_rwlock.write_lock();
    offset_map.clear();
    blobSize = 0;
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

    /*
     * Free each blob's catalog cache
     */
    for (BlobMap::iterator it = blobMap.begin();
         it != blobMap.end();
         it++) {
        delete it->second;
        it->second = NULL;
    }
    blobMap.clear();
}

Error VolumeCatalogCache::queryDm(const std::string& blobName,
                                  fds_uint64_t blobOffset,
                                  fds_uint32_t trans_id,
                                  ObjectID *oid) {
    Error err(ERR_OK);
    int   ret_code = 0;

    /*
     * Not in the cache. Contact DM.
     */
    LOGDEBUG << "VolumeCatalogCache - Cache query for volume 0x"
              << std::hex << vol_id << std::dec << " blob " << blobName
              << " offset " << blobOffset
              << " failed. Contacting DM.";

    /*
     * Construct the message to send to DM.
     */
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr
            (new FDS_ProtocolInterface::FDSP_MsgHdrType);
    FDS_ProtocolInterface::FDSP_QueryCatalogTypePtr query_req
            (new FDS_ProtocolInterface::FDSP_QueryCatalogType);

    msg_hdr->glob_volume_id = vol_id;
    msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_QUERY_CAT_OBJ_REQ;
    msg_hdr->src_id   = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
    msg_hdr->src_node_name = storHvisor->my_node_name;
    msg_hdr->dst_id   = FDS_ProtocolInterface::FDSP_DATA_MGR;
    msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
    msg_hdr->err_code = ERR_OK;
    msg_hdr->req_cookie = trans_id;

    /*
     * TODO: We should eventually specify the offset in the blob
     * we want...all objects won't work well for large blobs.
     */
    query_req->blob_name             = blobName;
    // We don't currently specify a version
    query_req->blob_version          = blob_version_invalid;
    query_req->dm_transaction_id     = 1;
    query_req->dm_operation          =
            FDS_ProtocolInterface::FDS_DMGR_TXN_STATUS_OPEN;
    query_req->obj_list.clear();
    query_req->meta_list.clear();

    /*
     * Locate a DM endpoint to try.
     */
    netSession *endPoint = NULL;
    DmtColumnPtr node_ids = parent_sh->dataPlacementTbl->getDMTNodesForVolume(vol_id);

    fds_uint32_t node_ip;
    fds_uint32_t node_port;
    int node_state;
    fds_bool_t located_ep = false;
    // send the query request to the Primary DM only. we may have to revisit this logic
    err = parent_sh->dataPlacementTbl->getNodeInfo((node_ids->get(0)).uuid_get_val(),
                                                   &node_ip,
                                                   &node_port,
                                                   &node_state);
    if (!err.ok()) {
        FDS_PLOG(vcc_log) << "VolumeCatalogCache - "
                          << "Unable to get node info for node " << std::hex
                          << (node_ids->get(0)).uuid_get_val() << std::dec << " " << err;
        return err;
    }

    netMetaDataPathClientSession *sessionCtx =  nullptr;
    try {
        sessionCtx = storHvisor->rpcSessionTbl->getClientSession<netMetaDataPathClientSession>(node_ip, node_port); //NOLINT
        if (sessionCtx == NULL) {
            LOGCRITICAL << "unable to get a client session to [" << node_ip << ":" << node_port << "]"; //NOLINT
            return ERR_NETWORK_TRANSPORT;
        }
        boost::shared_ptr<FDSP_MetaDataPathReqClient> client = sessionCtx->getClient();
        msg_hdr->session_uuid = sessionCtx->getSessionId();
        client->QueryCatalogObject(msg_hdr, query_req);
    } catch(const att::TTransportException& e) {
        LOGERROR << "error during network call : " << e.what();
        if (sessionCtx) {
            // TODO(prem) - WIN-301
            // storHvisor->rpcSessionTbl->endSession(sessionCtx);
        }
        return ERR_NETWORK_TRANSPORT;
    }

    LOGDEBUG << " VolumeCatalogCache - "
             << "Async query request sent to DM "
             << endPoint << " for volume " << std::hex << vol_id << std::dec
             << " and block id " << blobOffset << "Node IP: " << node_ip
             << " Node Port: " << node_port;
    /*
     * Reset the error to PENDING since we set the message.
     * This lets the caller know to wait for a response.
     */
    err = ERR_PENDING_RESP;

    return err;
}

bool VolumeCatalogCache::LookupObjectId(const std::string &blobName,
                                   const uint64_t &offset, ObjectID &objId)
{
    Error err(ERR_OK);

    blobRwLock.read_lock();

    if (blobMap.count(blobName) == 0) {
        blobRwLock.read_unlock();
        return false;
    } else {
        CatalogCache *catCache = blobMap.at(blobName);
        fds_verify(catCache != NULL);
        blobRwLock.read_unlock();

        err = catCache->Query(offset, &objId);
        if (!err.ok()) {
            return false;
        }
    }

    return true;
}

Error VolumeCatalogCache::Query(const std::string& blobName,
                                fds_uint64_t blobOffset,
                                fds_uint32_t trans_id,
                                ObjectID *oid) {
    Error err(ERR_OK);

    blobRwLock.read_lock();
    if (blobMap.count(blobName) == 0) {
        blobRwLock.read_unlock();
        /*
         * We don't know about this blob...query DM.
         */
        return queryDm(blobName, blobOffset, trans_id, oid);
    } else {
        CatalogCache *catCache = blobMap.at(blobName);
        fds_verify(catCache != NULL);
        blobRwLock.read_unlock();

        err = catCache->Query(blobOffset, oid);
        if (!err.ok()) {
            /*
             * Cache didn't have this offset...query DM.
             */
            return queryDm(blobName, blobOffset, trans_id, oid);
        }
    }

    return err;
}

/*
 * Update interface for a blob representing a block device.
 */
Error VolumeCatalogCache::Update(const std::string& blobName,
                                 fds_uint64_t blobOffset,
                                 fds_uint32_t objectLen,
                                 const ObjectID &oid,
                                 fds_bool_t lastBuf) {
    Error err(ERR_OK);
    CatalogCache *catCache;

    /*
     * Just grab a write lock for now.
     * The shared lock upgrade needs some work...
     */
    blobRwLock.write_lock();
    if (blobMap.count(blobName) == 0) {
        catCache = new CatalogCache(blobName);
        blobMap[blobName] = catCache;
        blobRwLock.write_unlock();
    } else {
        catCache = blobMap.at(blobName);
        blobRwLock.write_unlock();
    }

    err = catCache->Update(blobOffset, objectLen, oid, lastBuf);
    /*
     * If the exact entry already existed
     * we can return success.
     */
    if (err == ERR_DUPLICATE) {
        /*
         * Reset the error since it's OK
         * that is already existed.
         */
        LOGDEBUG << "VolumeCatalogCache - "
                 << "Cache update successful for volume 0x"
                 << std::hex << vol_id << std::dec << " blob offset "
                 << blobOffset << " was duplicate.";
        err = ERR_OK;
    } else if (err != ERR_OK) {
        LOGDEBUG << "VolumeCatalogCache - "
                 << "Cache update for volume 0x" << std::hex << vol_id
                 << std::dec << " blob offset " << blobOffset << " was failed!";
    } else {
        LOGDEBUG << "VolumeCatalogCache - "
                 << "Cache update successful for volume 0x" << std::hex
                 << vol_id << std::dec << " blob offset " << blobOffset;
    }

    LOGDEBUG << " VolumeCatalogCache"
             << " update DONE for blob " << blobName
             << " and blob offset " << blobOffset;
    return err;
}

Error VolumeCatalogCache::getBlobSize(const std::string& blobName,
                                      fds_uint64_t *blobSize) {
    Error err(ERR_OK);

    blobRwLock.write_lock();
    if (blobMap.count(blobName) > 0) {
        CatalogCache *blobCache = blobMap[blobName];
        *blobSize = blobCache->getBlobSize();
    } else {
        err = ERR_NOT_FOUND;
    }
    blobRwLock.write_unlock();

    return err;
}

Error VolumeCatalogCache::getBlobEtag(const std::string& blobName,
                                      std::string *blobEtag) {
    Error err(ERR_OK);

    blobRwLock.write_lock();
    if (blobMap.count(blobName) > 0) {
        CatalogCache *blobCache = blobMap[blobName];
        *blobEtag = blobCache->getBlobEtag();
    } else {
        err = ERR_NOT_FOUND;
    }
    blobRwLock.write_unlock();

    return err;
}

Error VolumeCatalogCache::setBlobEtag(const std::string& blobName,
                                      const std::string &blobEtag) {
    Error err(ERR_OK);

    blobRwLock.write_lock();
    if (blobMap.count(blobName) == 0) {
        err = ERR_NOT_FOUND;
    } else {
        CatalogCache *blobCache = blobMap[blobName];
        blobCache->setBlobEtag(blobEtag);
    }
    blobRwLock.write_unlock();

    return err;
}

/**
 * Clears a single blob entry from the cache. This
 * simply removes the in memory cache entry.
 * @return returns result of the clear. The function is
 * meant to be idempotent and return success even if the
 * entry didn't exist.
 */
Error VolumeCatalogCache::clearEntry(const std::string &blobName) {
    Error err(ERR_OK);

    blobRwLock.write_lock();
    if (blobMap.count(blobName) > 0) {
        CatalogCache *blobCache = blobMap[blobName];
        blobMap.erase(blobName);
        delete blobCache;
    }
    blobRwLock.write_unlock();

    return err;
}

/**
 * Clears the local in-memory cache for a volume.
 */
void VolumeCatalogCache::Clear() {
    blobRwLock.read_lock();
    for (BlobMap::iterator it = blobMap.begin();
         it != blobMap.end();
         it++) {
        it->second->Clear();
    }
    blobRwLock.read_unlock();

    LOGDEBUG << "VolumeCatalogCache - Cleared cache for volume 0x"
             << std::hex << vol_id << std::dec;
}

}  // namespace fds
