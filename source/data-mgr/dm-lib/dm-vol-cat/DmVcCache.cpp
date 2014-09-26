/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <vector>
#include <list>
#include <utility>
#include <fds_process.h>
#include <dm-vol-cat/DmVcCache.h>

namespace fds {

DmCacheVolCatalog::DmCacheVolCatalog(const std::string &modName)
        : Module(modName.c_str()) {
    volCacheMgr = std::unique_ptr<VolumeBlobCacheManager>(
        new VolumeBlobCacheManager("DM Volume cache manager"));

    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.dm.");
    maxEntries = conf.get<fds_uint32_t>("cache.default_max_entries");
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

    SCOPEDREAD(dmCacheRwLock);

    // Lookup extent key 0
    ExtentKey eKey(blob_name, extent_id);
    BlobExtent *extentPtr = NULL;

    // Get a pointer to extent from the cache
    error = volCacheMgr->get(volume_id,
                             eKey,
                             &extentPtr);
    if (error == ERR_OK) {
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
DmCacheVolCatalog::putExtent(fds_volid_t volume_id,
                             const std::string& blob_name,
                             const BlobExtent::const_ptr& extent) {
    std::vector<BlobExtent::const_ptr> extents;
    if (extent->getExtentId() == 0) {
        return putExtents(volume_id,
                          blob_name,
                          boost::dynamic_pointer_cast<const BlobExtent0>(extent),
                          extents);
    }

    extents.push_back(extent);
    return putExtents(volume_id, blob_name, NULL, extents);
}

Error
DmCacheVolCatalog::putExtents(fds_volid_t volume_id,
                              const std::string& blob_name,
                              const BlobExtent0::const_ptr& meta_extent,
                              const std::vector<BlobExtent::const_ptr>& extents) {
    // Iterate the extents and build a list of pointers to cache
    std::list<std::pair<ExtentKey, BlobExtent *>> extentPairPtrs;
    // Copy the meta extent if it exists
    if (meta_extent != NULL) {
        extentPairPtrs.push_back(
            std::make_pair(ExtentKey(blob_name, 0),
                           new BlobExtent0(*meta_extent)));
    }
    for (std::vector<BlobExtent::const_ptr>::const_iterator cit = extents.begin();
         cit != extents.end();
         cit++) {
        // Copy the extent into a new pointer and push that into
        // the list to pass the cache.
        // TODO(Andrew): We copy now because the data given to cache
        // isn't actually shared. It needs its own copy.
        extentPairPtrs.push_back(
            std::make_pair(ExtentKey(blob_name, (*cit)->getExtentId()),
                           new BlobExtent(
                               *((*cit).get()))));
    }
    SCOPEDWRITE(dmCacheRwLock);
    volCacheMgr->addBatch(volume_id, extentPairPtrs);

    return ERR_OK;
}

Error
DmCacheVolCatalog::removeExtent(fds_volid_t volume_id,
                                const std::string& blob_name,
                                fds_extent_id extent_id) {
    ExtentKey key(blob_name, extent_id);
    SCOPEDWRITE(dmCacheRwLock);
    return volCacheMgr->remove(volume_id, key);
}

}  // namespace fds
