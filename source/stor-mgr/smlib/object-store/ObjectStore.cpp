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
ObjectStore::addVolume(const VolumeDesc& voldesc) {
    Error err(ERR_OK);

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
ObjectDataStore::mod_init(SysParams const *const p) {
    Module::mod_init(p);
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
