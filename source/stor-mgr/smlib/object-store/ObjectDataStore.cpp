/*
 * Copyright 2014 Formation Data Systems, Inc. 
 */

#include <string>
#include <PerfTrace.h>
#include <object-store/ObjectDataStore.h>

namespace fds {

ObjectDataStore::ObjectDataStore(const std::string &modName)
        : Module(modName.c_str()) {
    dataCache = ObjectDataCache::unique_ptr(new ObjectDataCache("SM Object Data Cache"));
}

ObjectDataStore::~ObjectDataStore() {
}

Error
ObjectDataStore::putObjectData(fds_volid_t volId,
                               const ObjectID &objId,
                               diskio::DataTier tier,
                               boost::shared_ptr<const std::string> objData,
                               obj_phy_loc_t& objPhyLoc) {
    Error err(ERR_OK);

    // Construct persistent layer request
    meta_vol_io_t    vio;
    meta_obj_id_t    oid;
    fds_bool_t       sync = true;
    // TODO(Andrew): Should take a shared_ptr not a raw object buf
    ObjectBuf objBuf(*objData);
    memcpy(oid.metaDigest, objId.GetId(), objId.GetLen());
    SmPlReq *plReq =
            new SmPlReq(vio, oid,
                        const_cast<ObjectBuf *>(&objBuf),
                        sync, tier);

    { // scope for perf counter
        PerfContext tmp_pctx(SM_OBJ_DATA_DISK_WRITE,
                             volId, PerfTracer::perfNameStr(volId));
        SCOPED_PERF_TRACEPOINT_CTX(tmp_pctx);
        err = diskMgr->disk_write(plReq);
    }

    // Place the data in the cache
    if (err.ok()) {
        LOGDEBUG << "Wrote " << objId << " to persistent layer";
        if (tier == diskio::flashTier) {
            PerfTracer::incr(PUT_SSD_OBJ, volId, PerfTracer::perfNameStr(volId));
        }

        // get location in persistent layer to return with this method
        obj_phy_loc_t* loc = plReq->req_get_phy_loc();
        // copy to objPhyLoc because plReq will be freed as soon as we return
        memcpy(&objPhyLoc, loc, sizeof(obj_phy_loc_t));

        dataCache->putObjectData(volId, objId, objData);
        LOGDEBUG << "Wrote " << objId << " to cache";
    } else {
        LOGERROR << "Failed to write " << objId << " to persistent layer: " << err;
    }

    return err;
}

boost::shared_ptr<const std::string>
ObjectDataStore::getObjectData(fds_volid_t volId,
                               const ObjectID &objId,
                               ObjMetaData::const_ptr objMetaData,
                               Error &err) {
    // Check the cache for the object
    boost::shared_ptr<const std::string> objCachedData
            = dataCache->getObjectData(volId, objId, err);
    if (err.ok()) {
        LOGDEBUG << "Got " << objId << " from cache";
        PerfTracer::incr(SM_OBJ_DATA_CACHE_HIT, volId, PerfTracer::perfNameStr(volId));
        return objCachedData;
    }

    // Construct persistent layer request
    meta_vol_io_t   vio;
    meta_obj_id_t   oid;
    fds_bool_t      sync = true;
    diskio::DataTier tier;
    ObjectBuf       objBuf;
    memcpy(oid.metaDigest, objId.GetId(), objId.GetLen());
    SmPlReq *plReq = new SmPlReq(vio,
                                 oid,
                                 static_cast<ObjectBuf *>(&objBuf),
                                 sync);

    // read object from flash if we can
    if (objMetaData->onFlashTier()) {
        tier = diskio::flashTier;
    } else {
        tier = diskio::diskTier;
    }
    plReq->setTier(tier);
    plReq->set_phy_loc(objMetaData->getObjPhyLoc(tier));
    objBuf.size = objMetaData->getObjSize();
    objBuf.data.resize(objBuf.size, 0);

    {  // scope for perf counter
        PerfContext tmp_pctx(SM_OBJ_DATA_DISK_READ,
                             volId, PerfTracer::perfNameStr(volId));
        SCOPED_PERF_TRACEPOINT_CTX(tmp_pctx);
        err = diskMgr->disk_read(plReq);
    }
    if (err.ok()) {
        LOGDEBUG << "Got " << objId << " from persistent layer "
                 << " tier " << tier << " volume " << std::hex
                 << volId << std::dec;
        if (tier == diskio::flashTier) {
            PerfTracer::incr(GET_SSD_OBJ, volId, PerfTracer::perfNameStr(volId));
        }

        // Copy the data to the give buffer
        // TODO(Andrew): Remove the ObjectBuf concept and just pass the
        // data pointer directly to the persistent layer so that this
        // copy can be avoided.
        boost::shared_ptr<const std::string> objData(new std::string(objBuf.data));
        return objData;
    } else {
        LOGERROR << "Failed to get " << objId << " from persistent layer: " << err;
    }

    return NULL;
}

Error
ObjectDataStore::removeObjectData(fds_volid_t volId,
                                  const ObjectID& objId,
                                  const ObjMetaData::const_ptr& objMetaData) {
    Error err(ERR_OK);
    meta_obj_id_t   oid;
    LOGDEBUG << "Refcnt=0 for object id " << objId;

    // remove from data cache
    dataCache->removeObjectData(volId, objId);

    // tell persistent layer we deleted the object so that garbage collection
    // knows how much disk space we need to clean
    memcpy(oid.metaDigest, objId.GetId(), objId.GetLen());
    if (objMetaData->onTier(diskio::diskTier)) {
        diskMgr->disk_delete_obj(&oid, objMetaData->getObjSize(),
                                objMetaData->getObjPhyLoc(diskio::diskTier));
    } else if (objMetaData->onTier(diskio::flashTier)) {
        diskMgr->disk_delete_obj(&oid, objMetaData->getObjSize(),
                                objMetaData->getObjPhyLoc(diskio::flashTier));
    }

    return err;
}

/**
 * Module initialization
 */
int
ObjectDataStore::mod_init(SysParams const *const p) {
    Module::mod_init(p);
    diskMgr = &(diskio::DataIO::disk_singleton());
    dataCache->mod_init(p);
    LOGDEBUG << "Done.";
    return 0;
}

/**
 * Module startup
 */
void
ObjectDataStore::mod_startup() {
}

/**
 * Module shutdown
 */
void
ObjectDataStore::mod_shutdown() {
}

}  // namespace fds
