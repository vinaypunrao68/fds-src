/*
 * Copyright 2014 Formation Data Systems, Inc. 
 */

#include <string>
#include <map>
#include <ObjectId.h>
#include <fds_process.h>
#include <PerfTrace.h>
#include <TokenCompactor.h>
#include <object-store/ObjectStore.h>

namespace fds {

ObjectStore::ObjectStore(const std::string &modName,
                         StorMgrVolumeTable* volTbl)
        : Module(modName.c_str()),
          volumeTbl(volTbl),
          conf_verify_data(true),
          numBitsPerToken(0),
          diskMap(new SmDiskMap("SM Disk Map Module")),
          dataStore(new ObjectDataStore("SM Object Data Storage Module")),
          metaStore(new ObjectMetadataStore(
              "SM Object Metadata Storage Module")) {
}

ObjectStore::~ObjectStore() {
}

void
ObjectStore::handleNewDlt(const DLT* dlt) {
    fds_uint32_t nbits = dlt->getNumBitsForToken();
    metaStore->setNumBitsPerToken(nbits);

    Error err = diskMap->handleNewDlt(dlt);
    fds_verify(err.ok());

    // open metadata store for tokens owned by this SM
    err = metaStore->openMetadataStore(diskMap);
    fds_verify(err.ok());

    err = dataStore->openDataStore(diskMap);
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
    ScopedSynchronizer scopedLock(*taskSynchronizer, objId);

    Error err(ERR_OK);
    diskio::DataTier useTier = diskio::maxTier;
    GLOGTRACE << "Putting object " << objId;

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

        PerfTracer::incr(DUPLICATE_OBJ, volId, PerfTracer::perfNameStr(volId));
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
        // TODO(Anna) call tierEngine->selectTier(objId, volId);
        useTier = diskio::diskTier;
        obj_phy_loc_t objPhyLoc;  // will be set by data store
        err = dataStore->putObjectData(volId, objId, useTier, objData, objPhyLoc);
        if (!err.ok()) {
            LOGERROR << "Failed to write " << objId << " to obj data store "
                     << err;
            return err;
        }

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
    if (err.ok() && (useTier == diskio::flashTier)) {
        // TODO(Anna) if media policy not ssd, add to dirty flash list
    }

    return err;
}

boost::shared_ptr<const std::string>
ObjectStore::getObject(fds_volid_t volId,
                       const ObjectID &objId,
                       Error& err) {
    ScopedSynchronizer scopedLock(*taskSynchronizer, objId);

    // Get metadata from metadata store
    ObjMetaData::const_ptr objMeta = metaStore->getObjectMetadata(volId, objId, err);
    if (!err.ok()) {
        LOGERROR << "Failed to get object metadata" << objId << " volume "
                 << std::hex << volId << std::dec << " " << err;
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

    return objData;
}

Error
ObjectStore::deleteObject(fds_volid_t volId,
                          const ObjectID &objId) {
    ScopedSynchronizer scopedLock(*taskSynchronizer, objId);

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

    return err;
}

Error
ObjectStore::copyAssociation(fds_volid_t srcVolId,
                             fds_volid_t destVolId,
                             const ObjectID& objId) {
    ScopedSynchronizer scopedLock(*taskSynchronizer, objId);
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
                                     diskio::DataTier tier) {
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

        // write to object data store (will automatically write to new file)
        obj_phy_loc_t objPhyLoc;  // will be set by data store with new location
        err = dataStore->putObjectData(unknownVolId, objId, tier, objData, objPhyLoc);
        if (!err.ok()) {
            LOGERROR << "Failed to write " << objId << " to obj data store "
                     << ", tier " << tier << " " << err;
            return err;
        }

        // Create new object metadata to update physical location
        updatedMeta.reset(new ObjMetaData(objMeta));
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

/**
 * Module initialization
 */
int
ObjectStore::mod_init(SysParams const *const p) {
    static Module *objStoreDepMods[] = {
        diskMap.get(),
        dataStore.get(),
        metaStore.get(),
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
}

/**
 * Module shutdown
 */
void
ObjectStore::mod_shutdown() {
}

}  // namespace fds
