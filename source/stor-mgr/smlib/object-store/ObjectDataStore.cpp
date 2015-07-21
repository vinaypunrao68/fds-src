/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <PerfTrace.h>
#include <SmCtrl.h>
#include <fds_module_provider.h>
#include <object-store/ObjectDataStore.h>

namespace fds {

ObjectDataStore::ObjectDataStore(const std::string &modName,
                                 SmIoReqHandler *data_store)
        : Module(modName.c_str()),
          persistData(new ObjectPersistData("SM Obj Persist Data Store", data_store))
{
    dataCache = ObjectDataCache::unique_ptr(new ObjectDataCache("SM Object Data Cache"));
}

ObjectDataStore::~ObjectDataStore() {
    // Kill scavenger

    // This will close any leveDBs
    persistData.reset();
}

Error
ObjectDataStore::openDataStore(SmDiskMap::ptr& diskMap,
                               fds_bool_t pristineState) {
    return persistData->openObjectDataFiles(diskMap, pristineState);
}

Error
ObjectDataStore::openDataStore(SmDiskMap::ptr& diskMap,
                               const SmTokenSet& smToks,
                               fds_bool_t pristineState) {
    return persistData->openObjectDataFiles(diskMap, smToks, pristineState);
}

Error
ObjectDataStore::closeAndDeleteSmTokensStore(const SmTokenSet& smTokensLost) {
    return persistData->closeAndDeleteObjectDataFiles(smTokensLost);
}

Error
ObjectDataStore::deleteObjectDataFile(const std::string& diskPath,
                                      const fds_token_id& smToken,
                                      const fds_uint16_t& diskId) {
    return persistData->deleteObjectDataFile(diskPath, smToken, diskId);
}

Error
ObjectDataStore::putObjectData(fds_volid_t volId,
                               const ObjectID &objId,
                               diskio::DataTier tier,
                               boost::shared_ptr<const std::string>& objData,
                               obj_phy_loc_t& objPhyLoc) {
    Error err(ERR_OK);

    // Construct persistent layer request
    meta_vol_io_t    vio;
    meta_obj_id_t    oid;
    fds_bool_t       sync = true;
    // TODO(Anna): we also dob't need meta_obj_id_t structure in disk
    // request, should be able to use ObjectID type
    // TODO(Anna) cast not pretty, I think we should change API to
    // have shared_ptr of non const string
    boost::shared_ptr<std::string> sameObjData =
            boost::const_pointer_cast<std::string>(objData);
    ObjectBuf objBuf(sameObjData);
    memcpy(oid.metaDigest, objId.GetId(), objId.GetLen());
    diskio::DiskRequest *plReq =
            new diskio::DiskRequest(vio, oid,
                            const_cast<ObjectBuf *>(&objBuf),
                            sync, tier);

    { // scope for perf counter
        PerfContext tmp_pctx(PerfEventType::SM_OBJ_DATA_DISK_WRITE, volId);
        SCOPED_PERF_TRACEPOINT_CTX(tmp_pctx);
        err = persistData->writeObjectData(objId, plReq);
    }

    // Place the data in the cache
    if (err.ok()) {
        LOGDEBUG << "Wrote " << objId << " to persistent layer";
        if (tier == diskio::flashTier) {
            PerfTracer::incr(PerfEventType::SM_OBJ_DATA_SSD_WRITE, volId);
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

    delete plReq;
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
        PerfTracer::incr(PerfEventType::SM_OBJ_DATA_CACHE_HIT, volId);
        return objCachedData;
    }

    // Construct persistent layer request
    meta_vol_io_t   vio;
    meta_obj_id_t   oid;
    fds_bool_t      sync = true;
    diskio::DataTier tier;
    ObjectBuf       objBuf;
    memcpy(oid.metaDigest, objId.GetId(), objId.GetLen());
    diskio::DiskRequest *plReq = new diskio::DiskRequest(vio,
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
    (objBuf.data)->resize(objMetaData->getObjSize(), 0);

    {  // scope for perf counter
        PerfContext tmp_pctx(PerfEventType::SM_OBJ_DATA_DISK_READ,
                             volId);
        SCOPED_PERF_TRACEPOINT_CTX(tmp_pctx);
        err = persistData->readObjectData(objId, plReq);
    }
    if (err.ok()) {
        LOGDEBUG << "Got " << objId << " from persistent layer "
                 << " tier " << tier << " volume " << std::hex
                 << volId << std::dec;
        if (tier == diskio::flashTier) {
            PerfTracer::incr(PerfEventType::SM_OBJ_DATA_SSD_READ, volId);
        }

        // Copy the data to the give buffer
        // TODO(Andrew): Remove the ObjectBuf concept and just pass the
        // data pointer directly to the persistent layer so that this
        // copy can be avoided.
        boost::shared_ptr<std::string> objData(objBuf.data);
        delete plReq;
        return objData;
    } else {
        LOGERROR << "Failed to get " << objId << " from persistent layer: " << err;
        delete plReq;
    }

    return NULL;
}

Error
ObjectDataStore::removeObjectData(fds_volid_t volId,
                                  const ObjectID& objId,
                                  const ObjMetaData::const_ptr& objMetaData) {
    Error err(ERR_OK);
    LOGDEBUG << "Refcnt=0 for object id " << objId;

    // remove from data cache
    dataCache->removeObjectData(volId, objId);

    // tell persistent layer we deleted the object so that garbage collection
    // knows how much disk space we need to clean
    if (objMetaData->onTier(diskio::diskTier)) {
        persistData->notifyDataDeleted(objId, objMetaData->getObjSize(),
                                       objMetaData->getObjPhyLoc(diskio::diskTier));
    } else if (objMetaData->onTier(diskio::flashTier)) {
        persistData->notifyDataDeleted(objId, objMetaData->getObjSize(),
                                       objMetaData->getObjPhyLoc(diskio::flashTier));
    }

    return err;
}

Error
ObjectDataStore::scavengerControlCmd(SmScavengerCmd* scavCmd) {
    return persistData->scavengerControlCmd(scavCmd);
}

/**
 * Module initialization
 */
int
ObjectDataStore::mod_init(SysParams const *const p) {
    LOGDEBUG << "Will init Data Store";
    static Module *dataStoreDepMods[] = {
        dataCache.get(),
        persistData.get(),
        NULL
    };
    mod_intern = dataStoreDepMods;
    Module::mod_init(p);
    LOGDEBUG << "Done.";
    return 0;
}

/**
 * Module startup
 */
void
ObjectDataStore::mod_startup() {
    Module::mod_startup();
}

/**
 * Module shutdown
 */
void
ObjectDataStore::mod_shutdown() {
    Module::mod_shutdown();
    LOGDEBUG << "Done.";
}

}  // namespace fds
