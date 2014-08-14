/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <AmCache.h>

namespace fds {

AmCache::AmCache(const std::string &modName)
        : Module(modName.c_str()) {
    blobDescCache = std::unique_ptr<BlobDescCacheManager>(
        new BlobDescCacheManager("AM blob descriptor cache manager"));
}

AmCache::~AmCache() {
}

int
AmCache::mod_init(SysParams const *const param) {
    Module::mod_init(param);
    return 0;
}

void
AmCache::mod_startup() {
}

void
AmCache::mod_shutdown() {
}

Error
AmCache::createCache(const VolumeDesc& volDesc) {
    Error err = blobDescCache->createCache(volDesc.volUUID,
                                           maxEntries,
                                           evictionType);
    return err;
}

Error
AmCache::removeCache(fds_volid_t volId) {
    Error err = blobDescCache->deleteCache(volId);
    return err;
}

BlobDescriptor::ptr
AmCache::getBlobDescriptor(fds_volid_t volId,
                           const std::string &blobName,
                           Error &error) {
    LOGTRACE << "Cache lookup for volume " << std::hex << volId << std::dec
             << " blob " << blobName;

    BlobDescriptor *blobDescPtr = NULL;
    error = blobDescCache->get(volId, blobName, &blobDescPtr);
    if (error == ERR_OK) {
        // Copy the blob descriptor into an object we can safely
        // return to the caller
        BlobDescriptor::ptr blobDesc(new BlobDescriptor(*blobDescPtr));
        return blobDesc;
    }
    return NULL;
}

Error
AmCache::putTxDescriptor(const AmTxDescriptor::ptr &txDesc) {
    LOGTRACE << "Cache insert tx descriptor for volume " << std::hex
             << txDesc->volId << std::dec << " blob " << txDesc->blobName;

    // Add blob descriptor from tx to descriptor cache
    // TODO(Andrew): We copy now because the data given to cache
    // isn't actually shared. It needs its own copy.
    BlobDescriptor *cacheDesc = new BlobDescriptor(txDesc->stagedBlobDesc);
    std::unique_ptr<BlobDescriptor> evictedDesc =
            blobDescCache->add(cacheDesc->getVolId(),
                               cacheDesc->getBlobName(),
                               cacheDesc);
    if (evictedDesc != NULL) {
        LOGTRACE << "Evicted cached descriptor " << *evictedDesc;
    }

    return ERR_OK;
}

Error
AmCache::putBlobDescriptor(fds_volid_t volId,
                           const std::string &blobName,
                           const BlobDescriptor::ptr &blobDesc) {
    // Copy desc contents in structure owned by cache
    // TODO(Andrew): We copy now because the data given to cache
    // isn't actually shared. It needs its own copy.
    BlobDescriptor *cacheDesc = new BlobDescriptor(blobDesc);
    std::unique_ptr<BlobDescriptor> evictedDesc =
            blobDescCache->add(volId, blobName, cacheDesc);

    if (evictedDesc != NULL) {
        LOGTRACE << "Evicted cached descriptor " << *evictedDesc;
    }
    return ERR_OK;
}

Error
AmCache::removeBlob(fds_volid_t volId,
                    const std::string &blobName) {
    // Remove from blob descriptor cache
    Error err = blobDescCache->remove(volId, blobName);
    return err;
}

}  // namespace fds
