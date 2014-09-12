/*
 * Copyright 2014 Formation Data Systems, Inc. 
 */

#include <string>
#include <object-store/ObjectStore.h>

namespace fds {

ObjectStore::ObjectStore(const std::string &modName)
        : Module(modName.c_str()) {
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

    // Get metadata from store. Check data dedup and see if we
    // need to write to persistent layer.

    // Put data in store. We expect the data put to be atomic.
    // If we crash after the writing the data but before writing
    // the metadata, the orphaned object data will get cleaned up
    // on a subsequent scavenger pass.
    dataStore->putObjectData(volId, objId, objData);

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
