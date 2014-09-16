/*
 * Copyright 2014 Formation Data Systems, Inc. 
 */

#include <string>
#include <map>
#include <ObjectId.h>
#include <fds_process.h>
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
    GLOGTRACE << "Putting object " << objId;

    // New object metadata to update the refcnts
    ObjMetaData::ptr updatedMeta;

    // Get metadata from metadata store
    ObjMetaData::const_ptr objMeta;
    if (metaStore->getObjectMetadata(volId, objId, objMeta) == ERR_OK) {
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

        // TODO(Andrew): Make this a member variable that's set in the constructor based
        // on the config file
        fds_bool_t verify = false;
        if (verify == true) {
            // object already exists, check if duplicate
            // read object from object data store
            boost::shared_ptr<std::string> existObjData;
            // if we get an error, there are inconsistencies between
            // data and metadata; assert for now
            fds_verify(dataStore->getObjectData(volId, objId, objMeta, existObjData) == ERR_OK);

            // check if data is the same
            if (*existObjData != *objData) {
                // handle hash-collision
                if (conf_verify_data) {
                    // TODO(Andrew): Move to service layer...it's its job...
                    ObjectID putBufObjId;
                    putBufObjId = ObjIdGen::genObjectId(objData->c_str(),
                                                        objData->size());
                    LOGNORMAL << " Network-RPC ObjectId: " << putBufObjId.ToHex().c_str()
                              << " err  " << err;
                    if (putBufObjId != objId) {
                        err = ERR_NETWORK_CORRUPT;
                    }
                } else {
                    fds_panic("Encountered a hash collision checking object %s. Bailing out now!",
                              objId.ToHex().c_str());
                }
            }
        }  // if (verify == true)

        // data is duplicate since we had metadata already, update assoc entry for de-duping
        // TODO(Anna) update counter, should do in SM process layer, when err is ERR_DUPLICATE
        std::map<fds_volid_t, fds_uint32_t> vols_refcnt;
        objMeta->getVolsRefcnt(vols_refcnt);
        // Create new object metadata to update the refcnts
        updatedMeta.reset(new ObjMetaData(objMeta));
        updatedMeta->updateAssocEntry(objId, volId);
        volumeTbl->updateDupObj(volId,
                                objId,
                                objData->size(),
                                true,
                                vols_refcnt);
        err = ERR_DUPLICATE;
    } else {  // if (getMetadata == OK)
        updatedMeta.reset(new ObjMetaData());
    }

    // Put data in store if it's not a duplicate. We expect the data put to be atomic.
    // If we crash after the writing the data but before writing
    // the metadata, the orphaned object data will get cleaned up
    // on a subsequent scavenger pass.
    if (err.ok()) {
        // object not duplicate
        // TODO(Anna) call tier = tierEngine->selectTier(objId, volId);
        // and pass tier to dataStore
        err = dataStore->putObjectData(volId, objId, objData);
        if (!err.ok()) {
            LOGERROR << "Failed to write " << objId << " to obj data store "
                     << err;
            return err;
        }
    }

    // write metadata to metadata store
    fds_verify(err.ok() || (err == ERR_DUPLICATE));
    // update timestamps!
    // NOTE this assumes that ObjectStore::putObject only used for
    // data path (not backgroud process, etc)
    // TODO(Anna) pass int64_t origin timestamp as param to this method
    /*
    if (md.obj_map.obj_create_time == 0) {                                                                                                                                                    
        md.obj_map.obj_create_time = opCtx.ts;
    }
    md.obj_map.assoc_mod_time = opCtx.ts;
    */

    Error err_putmeta = metaStore->putObjectMetadata(volId, objId, updatedMeta);
    if (err_putmeta.ok() && err.ok()) {
        // data not dup and put data and metadata were successful
        // TODO(Anna) if written to flash, notify tier engine to add this obj for
        // write back thread
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

    return err;
}

/**
 * Module initialization
 */
int
ObjectStore::mod_init(SysParams const *const p) {
    Module::mod_init(p);

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
