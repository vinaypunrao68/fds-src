/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <dm-vol-cat/DmVcCache.h>

namespace fds {

DmCacheVolCatalog::DmCacheVolCatalog(const std::string &modName)
        : Module(modName.c_str()) {
    volCacheMgr = std::unique_ptr<VolumeBlobCacheManager>(
        new VolumeBlobCacheManager("DM Volume cache manager"));
}

DmCacheVolCatalog::~DmCacheVolCatalog() {
}

int
DmCacheVolCatalog::mod_init(SysParams const *const param) {
    Module::mod_init(param);
    return 0;
}

void
DmCacheVolCatalog::mod_startup() {
}

void
DmCacheVolCatalog::mod_shutdown() {
}

Error
DmCacheVolCatalog::createCache(const VolumeDesc& volDesc) {
    return volCacheMgr->createCache(volDesc.volUUID,
                                    maxEntries,
                                    evictionType);
}

Error
DmCacheVolCatalog::putMetaExtent(fds_volid_t volume_id,
                                 const std::string& blob_name,
                                 const BlobExtent0::const_ptr& meta_extent) {
    return ERR_OK;
}

}  // namespace fds
