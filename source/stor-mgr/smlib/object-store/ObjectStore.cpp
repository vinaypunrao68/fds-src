/*
 * Copyright 2014 Formation Data Systems, Inc. 
 */

#include <string>
#include <map>
#include <ObjectId.h>
#include <fds_process.h>
#include <PerfTrace.h>
#include <object-store/ObjectStore.h>

namespace fds {

ObjectStore::ObjectStore(const std::string &modName,
                         StorMgrVolumeTable* volTbl)
        : Module(modName.c_str()),
          volumeTbl(volTbl),
          conf_verify_data(true) {
    dataStore = ObjectDataStore::unique_ptr(
        new ObjectDataStore("SM Object Data Storage Module"));
}

ObjectStore::~ObjectStore() {
}

void
ObjectStore::setNumBitsPerToken(fds_uint32_t nbits) {
    if (metaStore) {
        metaStore->setNumBitsPerToken(nbits);
    }
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
            boost::shared_ptr<const std::string> existObjData;
            // if we get an error, there are inconsistencies between
            // data and metadata; assert for now
            fds_verify(dataStore->getObjectData(volId, objId, objMeta, existObjData) == ERR_OK);

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

Error
ObjectStore::getObject(fds_volid_t volId,
                       const ObjectID &objId,
                       boost::shared_ptr<std::string> objData) {
    Error err(ERR_OK);

    return err;
}

Error
ObjectStore::deleteObject(fds_volid_t volId,
                          const ObjectID &objId) {
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
    updatedMeta->deleteAssocEntry(objId, volId, fds::util::getTimeStampMillis());

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
        err = dataStore->removeObjectData(objId, updatedMeta);
    }

    return err;
}

/**
 * Module initialization
 */
int
ObjectStore::mod_init(SysParams const *const p) {
    Module::mod_init(p);

    dataStore->mod_init(p);

    const FdsRootDir *fdsroot = g_fdsprocess->proc_fdsroot();
    conf_verify_data = g_fdsprocess->get_fds_config()->get<bool>("fds.sm.data_verify");

    metaStore = ObjectMetadataStore::unique_ptr(
        new ObjectMetadataStore("SM Object Metadata Storage Module",
                            fdsroot->dir_user_repo_objs()));

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
