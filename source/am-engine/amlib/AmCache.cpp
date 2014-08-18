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
    blobOffsetCache = std::unique_ptr<BlobOffsetCacheManager>(
        new BlobOffsetCacheManager("AM blob offset cache manager"));
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
    if (err != ERR_OK) {
        return err;
    }
    err = blobOffsetCache->createCache(volDesc.volUUID,
                                       maxEntries,
                                       evictionType);
    return err;
}

Error
AmCache::removeCache(fds_volid_t volId) {
    Error err = blobDescCache->deleteCache(volId);
    if (err != ERR_OK) {
        return err;
    }
    err = blobOffsetCache->deleteCache(volId);
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

ObjectID::ptr
AmCache::getBlobOffsetObject(fds_volid_t volId,
                             const std::string &blobName,
                             fds_uint64_t blobOffset,
                             Error &error) {
    LOGTRACE << "Cache lookup for volume " << std::hex << volId << std::dec
             << " blob " << blobName << " offset " << blobOffset;

    ObjectID *blobObjectPtr = NULL;
    BlobOffsetPair offsetPair(blobName, blobOffset);
    error = blobOffsetCache->get(volId, offsetPair, &blobObjectPtr);
    if (error == ERR_OK) {
        // Copy the blob descriptor into an object we can safely
        // return to the caller
        ObjectID::ptr blobObjectId(new ObjectID(*blobObjectPtr));
        return blobObjectId;
    }
    return NULL;
}

Error
AmCache::putTxDescriptor(const AmTxDescriptor::ptr &txDesc) {
    LOGTRACE << "Cache insert tx descriptor for volume " << std::hex
             << txDesc->volId << std::dec << " blob " << txDesc->blobName;
    Error err(ERR_OK);

    // If the transaction is a delete, we want to remove the cache entry
    if (txDesc->opType == FDS_DELETE_BLOB) {
        // Remove from blob descriptor cache
        err = blobDescCache->remove(txDesc->volId,
                                    txDesc->blobName);
    } else {
        fds_verify(txDesc->opType == FDS_PUT_BLOB);
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

        // Add blob offsets from tx to offset cache
        for (const auto &offsetPair : txDesc->stagedBlobOffsets) {
            // TODO(Andrew): Allocate an objectId the cache can own.
            // We should change this to just take a pointer from the
            // transaction manager
            ObjectID *cacheObjId = new ObjectID(offsetPair.second);
            std::unique_ptr<ObjectID> evictedObjId =
                    blobOffsetCache->add(txDesc->volId,
                                         offsetPair.first,
                                         cacheObjId);
            if (evictedObjId != NULL) {
                LOGTRACE << "Evicted cached object id " << *evictedObjId;
            }
        }
    }
    return err;
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
