/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <AmCache.h>
#include <fds_process.h>
#include <PerfTrace.h>
#include <climits>
#include "AmTxDescriptor.h"

namespace fds {

AmCache::AmCache()
    : max_metadata_entries(0)
{
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    max_metadata_entries = conf.get<fds_uint32_t>("cache.max_metadata_entries");
}

AmCache::~AmCache() = default;

Error
AmCache::registerVolume(fds_volid_t const vol_uuid,
                        size_t const num_objs,
                        bool const can_cache_meta) {
    Error err;
    if (can_cache_meta) {
        err = descriptor_cache.addVolume(vol_uuid, max_metadata_entries);
        if (ERR_OK != err && ERR_VOL_DUPLICATE != err) {
            return err;
        }
        err = offset_cache.addVolume(vol_uuid, max_metadata_entries);
        if (ERR_OK != err && ERR_VOL_DUPLICATE != err) {
            descriptor_cache.removeVolume(vol_uuid);
            return err;
        }
    }
    err = object_cache.addVolume(vol_uuid, num_objs);
    if (ERR_OK != err && ERR_VOL_DUPLICATE != err) {
        offset_cache.removeVolume(vol_uuid);
        descriptor_cache.removeVolume(vol_uuid);
    }
    return err;
}

void
AmCache::invalidateMetaCache(fds_volid_t const volId) {
    offset_cache.clear(volId);
    descriptor_cache.clear(volId);
}

Error
AmCache::removeVolume(fds_volid_t const volId) {
    Error err = descriptor_cache.removeVolume(volId);
    if (err != ERR_OK) {
        return err;
    }
    err = offset_cache.removeVolume(volId);
    if (err != ERR_OK) {
        return err;
    }
    err = object_cache.removeVolume(volId);
    return err;
}

BlobDescriptor::ptr
AmCache::getBlobDescriptor(fds_volid_t volId,
                           const std::string &blobName,
                           Error &error) {
    LOGTRACE << "Cache lookup for volume " << std::hex << volId << std::dec
             << " blob " << blobName;

    BlobDescriptor::ptr blobDescPtr;
    error = descriptor_cache.get(volId, blobName, blobDescPtr);
    if (error == ERR_OK) {
        PerfTracer::incr(PerfEventType::AM_DESC_CACHE_HIT, volId);
    }
    return blobDescPtr;
}

ObjectID::ptr
AmCache::getBlobOffsetObject(fds_volid_t volId,
                             const std::string &blobName,
                             fds_uint64_t blobOffset,
                             Error &error) {
    LOGTRACE << "Cache lookup for volume " << std::hex << volId << std::dec
             << " blob " << blobName << " offset " << blobOffset;

    ObjectID::ptr blobOffsetPtr;
    error = offset_cache.get(volId, BlobOffsetPair(blobName, blobOffset), blobOffsetPtr);
    if (error == ERR_OK) {
        PerfTracer::incr(PerfEventType::AM_OFFSET_CACHE_HIT, volId);
    }
    return blobOffsetPtr;
}

boost::shared_ptr<std::string>
AmCache::getBlobObject(fds_volid_t volId,
                       const ObjectID &objectId,
                       Error &error) {
    LOGTRACE << "Cache lookup for volume " << std::hex << volId << std::dec
             << " object " << objectId;

    boost::shared_ptr<std::string> blobObjectPtr;
    error = object_cache.get(volId, objectId, blobObjectPtr);
    if (error == ERR_OK) {
        PerfTracer::incr(PerfEventType::AM_OBJECT_CACHE_HIT, volId);
    }
    return blobObjectPtr;
}

Error
AmCache::putBlobDescriptor(fds_volid_t const volId,
                           typename descriptor_cache_type::key_type const& blobName,
                           typename descriptor_cache_type::value_type const blobDesc)
{
    bool success;
    BlobDescriptor::ptr evictedDesc;
    std::tie(success, evictedDesc) = descriptor_cache.add(volId, blobName, blobDesc);
    if (evictedDesc) {
        GLOGTRACE << "Evicted cached descriptor " << *evictedDesc;
    }
    return success ? ERR_OK : ERR_VOL_NOT_FOUND;
}

Error
AmCache::putOffset(fds_volid_t const volId,
                   typename offset_cache_type::key_type const& blobOff,
                   typename offset_cache_type::value_type const objId)
{
    auto result = offset_cache.add(volId, blobOff, objId);
    return result.first ? ERR_OK : ERR_VOL_NOT_FOUND;
}

/**
 * Inserts new object into the object cache.
 */
Error
AmCache::putObject(fds_volid_t const volId,
                   typename object_cache_type::key_type const& objId,
                   typename object_cache_type::value_type const obj)
{
    auto result = object_cache.add(volId, objId, obj);
    return result.first ? ERR_OK : ERR_VOL_NOT_FOUND;
}

Error
AmCache::putTxDescriptor(const std::shared_ptr<AmTxDescriptor> txDesc, fds_uint64_t const blobSize) {
    LOGTRACE << "Cache insert tx descriptor for volume " << std::hex
             << txDesc->volId << std::dec << " blob " << txDesc->blobName;

    // If the transaction is a delete, we want to remove the cache entry
    if (txDesc->opType == FDS_DELETE_BLOB) {
        // Remove from blob caches
        removeBlob(txDesc->volId, txDesc->blobName);
    } else {
        fds_verify(txDesc->opType == FDS_PUT_BLOB);

        for (auto& offset_pair : txDesc->stagedBlobOffsets) {
            putOffset(txDesc->volId,
                      offset_pair.first,
                      boost::make_shared<ObjectID>(offset_pair.second));
        }

        // Add blob descriptor from tx to descriptor cache
        // TODO(Andrew): We copy now because the data given to cache
        // isn't actually shared. It needs its own copy.
        BlobDescriptor::ptr cacheDesc = txDesc->stagedBlobDesc;

        // Set the blob size to the one returned by DM
        cacheDesc->setBlobSize(blobSize);

        // Insert descriptor into the cache
        putBlobDescriptor(cacheDesc->getVolId(), cacheDesc->getBlobName(), cacheDesc);

        // Add blob objects from tx to object cache
        for (const auto &object : txDesc->stagedBlobObjects) {
            putObject(txDesc->volId, object.first, object.second);
        }
    }
    return ERR_OK;
}

Error
AmCache::removeBlob(fds_volid_t volId,
                    const std::string &blobName) {
    // Remove from blob descriptor cache
    Error err = descriptor_cache.remove(volId, blobName);

    // Remove from blob offset cache
    // TODO(Andrew): Need to be able to delete from the offset cache
    // using just blob name or using iterator

    // Remove from blob object cache
    // TODO(Andrew): Need to associate blobs to objects in the cache,
    // though for now it's safe to leave object IDs in the cache as
    // they won't be read if the blob offset cache is cleared on delete
    return err;
}

}  // namespace fds
