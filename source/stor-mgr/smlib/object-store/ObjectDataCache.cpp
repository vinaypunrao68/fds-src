/*
 * Copyright 2014 Formation Data Systems, Inc. 
 */

#include <string>
#include <fds_process.h>
#include <object-store/ObjectDataCache.h>

namespace fds {

ObjectDataCache::ObjectDataCache(const std::string &modName)
        : Module(modName.c_str()) {
}

ObjectDataCache::~ObjectDataCache() {
}

/**
 * Module initialization
 */
int
ObjectDataCache::mod_init(SysParams const *const p) {
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.sm.");
    maxEntries = conf.get<fds_uint32_t>("cache.default_data_entries");

    dataCache = std::unique_ptr<ObjectCache>(new ObjectCache("SM Object Data Cache",
                                                             maxEntries));

    Module::mod_init(p);
    return 0;
}

/**
 * Module startup
 */
void
ObjectDataCache::mod_startup() {
    Module::mod_startup();
}

/**
 * Module shutdown
 */
void
ObjectDataCache::mod_shutdown() {
    dataCache.reset();
    Module::mod_shutdown();
}

void
ObjectDataCache::putObjectData(fds_volid_t volId,
                               const ObjectID &objId,
                               boost::shared_ptr<const std::string> &objData) {
    if (maxEntries > 0) {
        dataCache->add(objId, objData);
    }
}

boost::shared_ptr<const std::string>
ObjectDataCache::getObjectData(fds_volid_t volId,
                               const ObjectID &objId,
                               Error& err) {
    // Query the cache and touch the entry
    boost::shared_ptr<const std::string> objData;
    if (maxEntries > 0) {
        err = dataCache->get(objId, objData);
    } else {
        err = ERR_NOT_FOUND;
    }
    return objData;
}

void
ObjectDataCache::removeObjectData(fds_volid_t volId,
                                  const ObjectID &objId) {
    // Query the cache and touch the entry
    if (maxEntries > 0) {
        dataCache->remove(objId);
    }
}

}  // namespace fds
