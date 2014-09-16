/*
 * Copyright 2014 Formation Data Systems, Inc. 
 */

#include <string>
#include <fds_process.h>
#include <object-store/ObjectMetaCache.h>

namespace fds {

ObjectMetaCache::ObjectMetaCache(const std::string &modName)
        : Module(modName.c_str()) {
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.sm.");
    maxEntries = conf.get<fds_uint32_t>("cache.default_meta_entries");

    metaCache =
            std::unique_ptr<MetadataCache>(new MetadataCache("SM Object Metadata Cache",
                                                             maxEntries));
}

ObjectMetaCache::~ObjectMetaCache() {
}

/**
 * Module initialization
 */
int
ObjectMetaCache::mod_init(SysParams const *const p) {
    Module::mod_init(p);
    return 0;
}

/**
 * Module startup
 */
void
ObjectMetaCache::mod_startup() {
}

/**
 * Module shutdown
 */
void
ObjectMetaCache::mod_shutdown() {
}

void
ObjectMetaCache::putObjectMetadata(fds_volid_t volId,
                                   const ObjectID &objId,
                                   ObjMetaData::const_ptr objMeta) {
    ObjMetaData::const_ptr evictedMeta
            = metaCache->add(objId, objMeta);
}

ObjMetaData::const_ptr
ObjectMetaCache::getObjectMetadata(fds_volid_t volId,
                                   const ObjectID &objId,
                                   Error &err) {
    // Query the cache and touch the entry
    ObjMetaData::const_ptr objMeta;
    err = metaCache->get(objId, objMeta);
    return objMeta;
}

void
ObjectMetaCache::removeObjectMetadata(fds_volid_t volId,
                                      const ObjectID &objId) {
    // Query the cache and touch the entry
    metaCache->remove(objId);
}

}  // namespace fds
