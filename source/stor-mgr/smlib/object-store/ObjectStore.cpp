/*
 * Copyright 2014 Formation Data Systems, Inc. 
 */

#include <string>
#include <map>
#include <ObjectId.h>
#include <fds_process.h>
#include <PerfTrace.h>
#include <object-store/TokenCompactor.h>
#include <object-store/ObjectStore.h>
#include <sys/statvfs.h>
#include <utility>
#include <object-store/TieringConfig.h>

namespace fds {

ObjectStore::ObjectStore(const std::string &modName,
                         SmIoReqHandler *data_store,
                         StorMgrVolumeTable* volTbl)
        : Module(modName.c_str()),
          volumeTbl(volTbl),
          conf_verify_data(true),
          numBitsPerToken(0),
          diskMap(new SmDiskMap("SM Disk Map Module")),
          dataStore(new ObjectDataStore("SM Object Data Storage", data_store)),
          metaStore(new ObjectMetadataStore(
              "SM Object Metadata Storage Module")),
          tierEngine(new TierEngine("SM Tier Engine",
                                    TierEngine::FDS_RANDOM_RANK_POLICY,
                                    volTbl, data_store)) {
}

ObjectStore::~ObjectStore() {
}

void
ObjectStore::handleNewDlt(const DLT* dlt) {
    fds_uint32_t nbits = dlt->getNumBitsForToken();
    metaStore->setNumBitsPerToken(nbits);

    Error err = diskMap->handleNewDlt(dlt);
    if (err == ERR_DUPLICATE) {
        return;  // everythin setup already
    } else if (err == ERR_INVALID_DLT) {
        return;  // we are ignoring this DLT
    }
    fds_verify(err.ok() || (err == ERR_SM_NOERR_PRISTINE_STATE));

    // open metadata store for tokens owned by this SM
    err = metaStore->openMetadataStore(diskMap);
    fds_verify(err.ok());

    err = dataStore->openDataStore(diskMap,
                                   (err == ERR_SM_NOERR_PRISTINE_STATE));
    fds_verify(err.ok());
}

Error
ObjectStore::addVolume(const VolumeDesc& volDesc) {
    Error err(ERR_OK);
    GLOGTRACE << "Adding volume " << volDesc;
    return err;
}

Error
ObjectStore::removeVolume(fds_volid_t volId) {
    Error err(ERR_OK);
    return err;
}

Error
ObjectStore::putObject(fds_volid_t volId,
                       const ObjectID &objId,
                       boost::shared_ptr<const std::string> objData) {
    PerfContext objWaitCtx(SM_PUT_OBJ_TASK_SYNC_WAIT, volId, PerfTracer::perfNameStr(volId));
    PerfTracer::tracePointBegin(objWaitCtx);
    ScopedSynchronizer scopedLock(*taskSynchronizer, objId);
    PerfTracer::tracePointEnd(objWaitCtx);

    Error err(ERR_OK);
    diskio::DataTier useTier = diskio::maxTier;
    LOGTRACE << "Putting object " << objId << " volume " << std::hex << volId
             << std::dec;

    // New object metadata to update the refcnts
    ObjMetaData::ptr updatedMeta;

    // Get metadata from metadata store
    ObjMetaData::const_ptr objMeta = metaStore->getObjectMetadata(volId, objId, err);
    if (err == ERR_OK) {
        // While sync is going on we can have metadata but object could be missing
        if (!objMeta->dataPhysicallyExists()) {
            // fds_verify(isTokenInSyncMode(getDLT()->getToken(objId)));
            LOGNORMAL << "Got metadata but not data " << objId
                      << " sync is in progress";
            err = ERR_SM_OBJECT_DATA_MISSING;
            // TODO(Anna) revisit this path when porting migration,
            // in the existing implementation, we would write new data to data store
            // and updating the existing metadata is this what we want?
            fds_panic("Missing data in the persistent layer!");  // implement
        }

        // check if existing object corrupted
        if (objMeta->isObjCorrupted()) {
            LOGCRITICAL << "Obj metadata indicates dup object corrupted, returning err";
            return ERR_SM_DUP_OBJECT_CORRUPT;
        }

        if (conf_verify_data == true) {
            // verify data -- read object from object data store
            boost::shared_ptr<const std::string> existObjData
                    = dataStore->getObjectData(volId, objId, objMeta, err);
            // if we get an error, there are inconsistencies between
            // data and metadata; assert for now
            fds_verify(err.ok());

            // check if data is the same
            if (*existObjData != *objData) {
                fds_panic("Encountered a hash collision checking object %s. Bailing out now!",
                          objId.ToHex().c_str());
            }
        }  // if (conf_verify_data == true)

        // Create new object metadata to update the refcnts
        updatedMeta.reset(new ObjMetaData(objMeta));

        PerfTracer::incr(SM_PUT_DUPLICATE_OBJ, volId, PerfTracer::perfNameStr(volId));
        err = ERR_DUPLICATE;
    } else {  // if (getMetadata != OK)
        // We didn't find any metadata, make sure it was just not there and reset
        fds_verify(err == ERR_NOT_FOUND);
        err = ERR_OK;
        updatedMeta.reset(new ObjMetaData());
        updatedMeta->initialize(objId, objData->size());
    }

    fds_verify(err.ok() || (err == ERR_DUPLICATE));

    // either new data or dup, update assoc entry
    std::map<fds_volid_t, fds_uint32_t> vols_refcnt;
    updatedMeta->getVolsRefcnt(vols_refcnt);
    updatedMeta->updateAssocEntry(objId, volId);
    volumeTbl->updateDupObj(volId,
                            objId,
                            objData->size(),
                            true,
                            vols_refcnt);

    // Put data in store if it's not a duplicate. We expect the data put to be atomic.
    // If we crash after the writing the data but before writing
    // the metadata, the orphaned object data will get cleaned up
    // on a subsequent scavenger pass.
    if (err.ok()) {
        // object not duplicate
        // select tier to put object
        StorMgrVolume *vol = volumeTbl->getVolume(volId);
        fds_verify(vol);
        useTier = tierEngine->selectTier(objId, *vol->voldesc);

        if (useTier == diskio::flashTier) {
            // TODO(brian): Has this actually saved us any accesses?
            // TODO(brian): Where do we get flashFullThreshold from still?
            fds_bool_t ssdSuccess = diskMap->ssdTrackCapacityAdd(objId,
                    objData->size(), tierEngine->getFlashFullThreshold());

            if (!ssdSuccess) {
                useTier = diskio::diskTier;
                diskMap->ssdTrackCapacityDelete(objId, objData->size());
            }
        }

        // put object to datastore
        obj_phy_loc_t objPhyLoc;  // will be set by data store
        err = dataStore->putObjectData(volId, objId, useTier, objData, objPhyLoc);
        if (!err.ok()) {
            LOGERROR << "Failed to write " << objId << " to obj data store "
                     << err;
            return err;
        }

        // Notify tier engine of recent IO
        tierEngine->notifyIO(objId, FDS_SM_PUT_OBJECT, *vol->voldesc, useTier);

        // update physical location that we got from data store
        updatedMeta->updatePhysLocation(&objPhyLoc);
    }

    // TODO(Anna) When we review SM -> SM migration, review if we need origin timestamp
    // Would need to pass int64_t origin timestamp as param to this method
    /*
    if (md.obj_map.obj_create_time == 0) {                                                                                                                                                    
        md.obj_map.obj_create_time = opCtx.ts;
    }
    md.obj_map.assoc_mod_time = opCtx.ts;
    */

    // write metadata to metadata store
    err = metaStore->putObjectMetadata(volId, objId, updatedMeta);

    return err;
}

boost::shared_ptr<const std::string>
ObjectStore::getObject(fds_volid_t volId,
                       const ObjectID &objId,
                       Error& err) {
    PerfContext objWaitCtx(SM_GET_OBJ_TASK_SYNC_WAIT, volId, PerfTracer::perfNameStr(volId));
    PerfTracer::tracePointBegin(objWaitCtx);
    ScopedSynchronizer scopedLock(*taskSynchronizer, objId);
    PerfTracer::tracePointEnd(objWaitCtx);

    // Get metadata from metadata store
    ObjMetaData::const_ptr objMeta = metaStore->getObjectMetadata(volId, objId, err);
    if (!err.ok()) {
        LOGERROR << "Failed to get object metadata" << objId << " volume "
                 << std::hex << volId << std::dec << " " << err;
        return NULL;
    }

    // check if object corrupted
    if (objMeta->isObjCorrupted()) {
        LOGCRITICAL << "Obj metadata indicates data corrupted, will return err";
        err = ERR_ONDISK_DATA_CORRUPT;
        return NULL;
    }

    /*
     * TODO(umesh): uncomment this when reference counting is used.
     *
    // If this Volume never put this object, then it should not access the object
    if (!objMeta->isVolumeAssociated(volId)) {
        err = ERR_NOT_FOUND;
        LOGWARN << "Volume " << std::hex << volId << std::dec << " aunauth access "
                << " to object " << objId << " returning " << err;
        return NULL;
    }
    */

    // get object data
    boost::shared_ptr<const std::string> objData
            = dataStore->getObjectData(volId, objId, objMeta, err);
    if (!err.ok()) {
        LOGERROR << "Failed to get object data " << objId << " volume "
                 << std::hex << volId << std::dec << " " << err;
        return objData;
    }

    // verify data
    if (conf_verify_data) {
        ObjectID onDiskObjId;
        onDiskObjId = ObjIdGen::genObjectId(objData->c_str(),
                                            objData->size());
        if (onDiskObjId != objId) {
            fds_panic("Encountered a on-disk data corruption object %s \n != %s",
                      objId.ToHex().c_str(), onDiskObjId.ToHex().c_str());
        }
    }

    // We are passing max_tier here, if we decide we
    // care which tier GET is from we need to change this
    tierEngine->notifyIO(objId, FDS_SM_GET_OBJECT,
            *volumeTbl->getVolume(volId)->voldesc, diskio::maxTier);

    return objData;
}

Error
ObjectStore::deleteObject(fds_volid_t volId,
                          const ObjectID &objId) {
    PerfContext objWaitCtx(SM_DELETE_OBJ_TASK_SYNC_WAIT, volId, PerfTracer::perfNameStr(volId));
    PerfTracer::tracePointBegin(objWaitCtx);
    ScopedSynchronizer scopedLock(*taskSynchronizer, objId);
    PerfTracer::tracePointEnd(objWaitCtx);

    Error err(ERR_OK);

    // New object metadata to update the refcnts
    ObjMetaData::ptr updatedMeta;

    // Get metadata from metadata store
    ObjMetaData::const_ptr objMeta =
            metaStore->getObjectMetadata(volId, objId, err);
    if (!err.ok()) {
        fds_verify(err == ERR_NOT_FOUND);
        LOGDEBUG << "Not able to read existing object locations, "
                 << "assuming no prior entry existed " << objId;
        return ERR_OK;
    }

    // if object corrupted, no point of updating metadata
    if (objMeta->isObjCorrupted()) {
        LOGCRITICAL << "Object corrupted, returning error ";
        return err = ERR_ONDISK_DATA_CORRUPT;
    }

    // Create new object metadata to update the refcnts
    updatedMeta.reset(new ObjMetaData(objMeta));
    std::map<fds_volid_t, fds_uint32_t> vols_refcnt;
    updatedMeta->getVolsRefcnt(vols_refcnt);
    // remove volume assoc entry
    fds_bool_t change = updatedMeta->deleteAssocEntry(objId,
                                                      volId,
                                                      fds::util::getTimeStampMillis());
    if (!change) {
        // the volume was not associated, ok
        LOGNORMAL << "Volume " << std::hex << volId << std::dec
                  << " is not associated with this obj " << objId
                  << ", nothing to delete, returning OK";
        return ERR_OK;
    }

    // first write metadata to metadata store, even if removing
    // object from data store cache fails, it is ok
    err = metaStore->putObjectMetadata(volId, objId, updatedMeta);
    if (err.ok()) {
        PerfTracer::incr(SM_OBJ_MARK_DELETED, volId, PerfTracer::perfNameStr(volId));
        volumeTbl->updateDupObj(volId,
                                objId,
                                updatedMeta->getObjSize(),
                                false,
                                vols_refcnt);
        LOGDEBUG << "Decremented refcnt for object " << objId
                 << " " << *updatedMeta << " refcnt: "
                 << updatedMeta->getRefCnt();
    } else {
        LOGERROR << "Failed to update metadata for object " << objId
                 << " " << err;
        return err;
    }

    // if refcnt is 0, notify data store so it can remove cache entry, etc
    if (updatedMeta->getRefCnt() < 1) {
        err = dataStore->removeObjectData(volId, objId, updatedMeta);
    }

    // notifying tier engine, change this if we decide we care which tier the delete happens from
    tierEngine->notifyIO(objId, FDS_SM_DELETE_OBJECT,
            *volumeTbl->getVolume(volId)->voldesc, diskio::maxTier);

    return err;
}

Error
ObjectStore::moveObjectToTier(const ObjectID& objId,
                              diskio::DataTier fromTier,
                              diskio::DataTier toTier,
                              fds_bool_t relocateFlag) {
    Error err(ERR_OK);
    ScopedSynchronizer scopedLock(*taskSynchronizer, objId);

    LOGDEBUG << "Moving object " << objId << " from tier " << fromTier
             << " to tier " << toTier << " relocate?" << relocateFlag;

    // since object can be associated with multiple volumes, we just
    // set volume id to invalid. Here is it used only to collect perf
    // stats and associate them with a volume; moving objects between
    // tiers will be accounted against volume 0
    fds_volid_t unknownVolId = invalid_vol_id;

    // Get metadata from metadata store
    ObjMetaData::const_ptr objMeta = metaStore->getObjectMetadata(unknownVolId, objId, err);
    if (!err.ok()) {
        LOGERROR << "Failed to get metadata for object " << objId << " " << err;
        return err;
    }

    // do not move if object is deleted
    if (objMeta->getRefCnt() < 1) {
        LOGWARN << "Object refcnt == 0";
        return err;
    }

    // make sure the object is not on destination tier already
    if (objMeta->onTier(toTier)) {
        LOGERROR << "Object " << objId << " is already on tier " << toTier;
        return ERR_DUPLICATE;
    }

    // read object from fromTier
    boost::shared_ptr<const std::string> objData
            = dataStore->getObjectData(unknownVolId, objId, objMeta, err);
    if (!err.ok()) {
        LOGERROR << "Failed to get object data " << objId << " for copying "
                 << "to tier " << toTier << " " << err;
        return err;
    }

    // write to object data store to toTier
    obj_phy_loc_t objPhyLoc;  // will be set by data store with new location
    err = dataStore->putObjectData(unknownVolId, objId, toTier, objData, objPhyLoc);
    if (!err.ok()) {
        LOGERROR << "Failed to write " << objId << " to obj data store "
                 << ", tier " << toTier << " " << err;
        return err;
    }

    // update physical location that we got from data store
    ObjMetaData::ptr updatedMeta(new ObjMetaData(objMeta));
    updatedMeta->updatePhysLocation(&objPhyLoc);
    if (relocateFlag) {
        // remove from fromTier
        updatedMeta->removePhyLocation(fromTier);
    }

    // write metadata to metadata store
    err = metaStore->putObjectMetadata(unknownVolId, objId, updatedMeta);
    if (!err.ok()) {
        LOGERROR << "Failed to update metadata for obj " << objId;
    }
    return err;
}

Error
ObjectStore::copyAssociation(fds_volid_t srcVolId,
                             fds_volid_t destVolId,
                             const ObjectID& objId) {
    PerfContext objWaitCtx(SM_ADD_OBJ_REF_TASK_SYNC_WAIT, destVolId,
                           PerfTracer::perfNameStr(destVolId));
    PerfTracer::tracePointBegin(objWaitCtx);
    ScopedSynchronizer scopedLock(*taskSynchronizer, objId);
    PerfTracer::tracePointEnd(objWaitCtx);

    Error err(ERR_OK);

    // New object metadata to update association
    ObjMetaData::ptr updatedMeta;

    // Get metadata from metadata store
    ObjMetaData::const_ptr objMeta = metaStore->getObjectMetadata(srcVolId, objId, err);
    if (!err.ok()) {
        LOGERROR << "Failed to get metadata for object " << objId << " " << err;
        return err;
    }

    // copy association entry
    updatedMeta.reset(new ObjMetaData(objMeta));
    std::map<fds_volid_t, fds_uint32_t> vols_refcnt;
    updatedMeta->getVolsRefcnt(vols_refcnt);
    updatedMeta->copyAssocEntry(objId, srcVolId, destVolId);

    // write updated metadata to meta store
    err = metaStore->putObjectMetadata(destVolId, objId, updatedMeta);
    if (err.ok()) {
        // TODO(xxx) we should call updateDupObj()
    } else {
        LOGERROR << "Failed to add association entry for object " << objId
                 << " to object metadata store " << err;
    }

    return err;
}

Error
ObjectStore::copyObjectToNewLocation(const ObjectID& objId,
                                     diskio::DataTier tier,
                                     fds_bool_t verifyData) {
    ScopedSynchronizer scopedLock(*taskSynchronizer, objId);
    Error err(ERR_OK);

    // since object can be associated with multiple volumes, we just
    // set volume id to invalid. Here is it used only to collect perf
    // stats and associate them with a volume; in case of GC, all metadata
    // and disk accesses will be counted against volume 0
    fds_volid_t unknownVolId = invalid_vol_id;

    // Get metadata from metadata store
    ObjMetaData::const_ptr objMeta = metaStore->getObjectMetadata(unknownVolId, objId, err);
    if (!err.ok()) {
        LOGERROR << "Failed to get metadata for object " << objId << " " << err;
        return err;
    }

    if (!TokenCompactor::isDataGarbage(*objMeta, tier)) {
        // this object is valid, copy it to a current token file
        ObjMetaData::ptr updatedMeta;
        LOGDEBUG << "Will copy " << objId << " to new file on tier " << tier;

        // first read the object
        boost::shared_ptr<const std::string> objData
                = dataStore->getObjectData(unknownVolId, objId, objMeta, err);
        if (!err.ok()) {
            LOGERROR << "Failed to get object data " << objId << " for copying "
                     << "to new file (not garbage collect) " << err;
            return err;
        }
        // Create new object metadata for update
        updatedMeta.reset(new ObjMetaData(objMeta));

        // we may be copying file with objects that already has 'corrupt'
        // flag set. Since we are not yet recovering corrupted objects, we
        // are going to copy corrupted objects to new files (not loose them)
        if (!objMeta->isObjCorrupted() && verifyData) {
            ObjectID onDiskObjId;
            onDiskObjId = ObjIdGen::genObjectId(objData->c_str(),
                                                objData->size());
            if (onDiskObjId != objId) {
                // on-disk data corruption
                // mark object metadata as corrupted! will copy it to new
                // location anyway so we can debug the issue
                LOGCRITICAL << "Encountered a on-disk data corruption object "
                            << objId.ToHex() << "!=" <<  onDiskObjId.ToHex();
                // set flag in object metadata
                updatedMeta->setObjCorrupted();
            }
        }

        // write to object data store (will automatically write to new file)
        obj_phy_loc_t objPhyLoc;  // will be set by data store with new location
        err = dataStore->putObjectData(unknownVolId, objId, tier, objData, objPhyLoc);
        if (!err.ok()) {
            LOGERROR << "Failed to write " << objId << " to obj data store "
                     << ", tier " << tier << " " << err;
            return err;
        }

        // update physical location that we got from data store
        updatedMeta->updatePhysLocation(&objPhyLoc);
        // write metadata to metadata store
        err = metaStore->putObjectMetadata(unknownVolId, objId, updatedMeta);
        if (!err.ok()) {
            LOGERROR << "Failed to update metadata for obj " << objId;
        }
    } else {
        // not going to copy object to new location
        LOGNORMAL << "Will garbage-collect " << objId << " on tier " << tier;
        // remove entry from index db if data + meta is garbage
        if (TokenCompactor::isGarbage(*objMeta)) {
            LOGNORMAL << "Removing metadata for " << objId;
            err = metaStore->removeObjectMetadata(unknownVolId, objId);
        }
    }

    return err;
}

void
ObjectStore::snapshotMetadata(fds_token_id smTokId,
                              SmIoSnapshotObjectDB::CbType notifFn) {
    metaStore->snapshot(smTokId, notifFn);
}

Error
ObjectStore::scavengerControlCmd(SmScavengerCmd* scavCmd) {
    return dataStore->scavengerControlCmd(scavCmd);
}

/**
 * Module initialization
 */
int
ObjectStore::mod_init(SysParams const *const p) {
    static Module *objStoreDepMods[] = {
        dataStore.get(),
        diskMap.get(),
        metaStore.get(),
        tierEngine.get(),
        NULL
    };
    mod_intern = objStoreDepMods;
    Module::mod_init(p);

    conf_verify_data = g_fdsprocess->get_fds_config()->get<bool>("fds.sm.data_verify");
    taskSyncSize =
            g_fdsprocess->get_fds_config()->get<fds_uint32_t>(
                "fds.sm.objectstore.synchronizer_size");
    taskSynchronizer = std::unique_ptr<HashedLocks<ObjectID, ObjectHash>>(
        new HashedLocks<ObjectID, ObjectHash>(taskSyncSize));

    LOGDEBUG << "Done";
    return 0;
}

/**
 * Module startup
 */
void
ObjectStore::mod_startup() {
    Module::mod_startup();
}

/**
 * Module shutdown
 */
void
ObjectStore::mod_shutdown() {
    Module::mod_shutdown();
}

}  // namespace fds
