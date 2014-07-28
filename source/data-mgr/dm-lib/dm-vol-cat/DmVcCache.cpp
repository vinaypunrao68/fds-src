/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <vector>
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

BlobExtent::ptr
DmCacheVolCatalog::getExtent(fds_volid_t volume_id,
                             const std::string& blob_name,
                             fds_extent_id extent_id,
                             Error& error) {
    LOGTRACE << "Cache lookup extent " << extent_id << " for "
             << std::hex << volume_id << std::dec << "," << blob_name;

    // Lookup extent key 0
    ExtentKey eKey(blob_name, extent_id);
    BlobExtent *extentPtr = NULL;

    // Get a pointer to extent from the cache
    error = volCacheMgr->get(volume_id,
                             eKey,
                             &extentPtr);
    if (error == ERR_OK) {
        // Make sure we read back the correct extent
        fds_verify(extentPtr->getExtentId() == 0);

        // Copy the extent into a object we can safely
        // return to the caller.
        // TODO(Andrew): Avoid this copy when we can instead
        // pin the element in the cache until the caller is done
        // with it

        BlobExtent::ptr extent;
        if (extent_id == 0) {
            extent.reset(new BlobExtent0(
                *(static_cast<BlobExtent0 *>(extentPtr))));
        } else {
            extent.reset(new BlobExtent(
                *extentPtr));
        }
        return extent;
    }

    return NULL;
}

Error
DmCacheVolCatalog::removeCache(fds_volid_t volId) {
    return volCacheMgr->deleteCache(volId);
}

Error
DmCacheVolCatalog::putExtents(fds_volid_t volume_id,
                              const std::string& blob_name,
                              const BlobExtent0::const_ptr& meta_extent,
                              const std::vector<BlobExtent::const_ptr>& extents) {
    // Iterate the extents and add them to the cache
    for (std::vector<BlobExtent::const_ptr>::const_iterator cit = extents.begin();
         cit != extents.end();
         cit++) {
    }

    return ERR_OK;
}

}  // namespace fds
