/*
 * Copyright 2014 Formation Data Systems, Inc. 
 */

#include <string>
#include <object-store/ObjectDataStore.h>

namespace fds {

ObjectDataStore::ObjectDataStore(const std::string &modName)
        : Module(modName.c_str()),
          diskMgr(&(diskio::DataIO::disk_singleton())) {
}

ObjectDataStore::~ObjectDataStore() {
}

Error
ObjectDataStore::createDataStore(fds_volid_t volId) {
    Error err(ERR_OK);

    return err;
}

Error
ObjectDataStore::removeDataStore(fds_volid_t volId) {
    Error err(ERR_OK);

    return err;
}

Error
ObjectDataStore::putObjectData(fds_volid_t volId,
                               const ObjectID &objId,
                               boost::shared_ptr<const std::string> objData) {
    Error err(ERR_OK);

    return err;
}

Error
ObjectDataStore::getObjectData(fds_volid_t volId,
                               const ObjectID &objId,
                               boost::shared_ptr<std::string> objData) {
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
