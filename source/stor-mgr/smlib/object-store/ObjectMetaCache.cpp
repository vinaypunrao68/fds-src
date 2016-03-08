/*
 * Copyright 2014 Formation Data Systems, Inc. 
 */

#include <string>
#include <fds_process.h>
#include <object-store/ObjectMetaCache.h>

namespace fds {

ObjectMetaCache::ObjectMetaCache(const std::string &modName)
        : Module(modName.c_str()) {
}

ObjectMetaCache::~ObjectMetaCache() {
}

/**
 * Module initialization
 */
int
ObjectMetaCache::mod_init(SysParams const *const p) {
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.sm.");
    maxEntries = conf.get<fds_uint32_t>("cache.default_meta_entries");

    metaCache =
            std::unique_ptr<MetadataCache>(new MetadataCache("SM Object Metadata Cache",
                                                             maxEntries));

    Module::mod_init(p);
    return 0;
}

/**
 * Module startup
 */
void
ObjectMetaCache::mod_startup() {
    Module::mod_startup();
}

/**
 * Module shutdown
 */
void
ObjectMetaCache::mod_shutdown() {
    metaCache.reset();
    Module::mod_shutdown();
}

void
ObjectMetaCache::putObjectMetadata(fds_volid_t volId,
                                   const ObjectID &objId,
                                   ObjMetaData::const_ptr objMeta) {
    if (maxEntries > 0) {
        metaCache->add(objId, objMeta);
    }
}

ObjMetaData::const_ptr
ObjectMetaCache::getObjectMetadata(fds_volid_t volId,
                                   const ObjectID &objId,
                                   Error &err) {
    // Query the cache and touch the entry
    ObjMetaData::const_ptr objMeta;
    if (maxEntries > 0) {
        err = metaCache->get(objId, objMeta);
    } else {
        err = ERR_NOT_FOUND;
    }
    return objMeta;
}

void
ObjectMetaCache::removeObjectMetadata(fds_volid_t volId,
                                      const ObjectID &objId) {
    // Query the cache and touch the entry
    if (maxEntries > 0) {
        metaCache->remove(objId);
    }
}

}  // namespace fds
