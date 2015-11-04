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
    max_metadata_entries = std::min((uint64_t)LLONG_MAX, (uint64_t)conf.get<int64_t>("cache.max_metadata_entries"));
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

Error
AmCache::getBlobOffsetObjects(fds_volid_t volId,
                              const std::string &blobName,
                              fds_uint64_t const obj_offset,
                              fds_uint64_t const obj_offset_end,
                              size_t const obj_size,
                              std::vector<ObjectID::ptr>& obj_ids) {
    LOGTRACE << "Cache lookup for volume " << std::hex << volId << std::dec
             << " blob " << blobName << " offset " << obj_offset
             << " objects " << obj_ids.size();

    // Search for all the Offset pairs for the range of data we're interested in
    // and populate the object ids vector with ids.
    obj_ids.clear();
    for (auto cur_off = obj_offset; obj_offset_end >= cur_off; cur_off += obj_size) {
        ObjectID::ptr obj_id;
        auto offset_pair = BlobOffsetPair(blobName, cur_off);
        auto err = offset_cache.get(volId, offset_pair, obj_id);
        if (err == ERR_OK) {
            PerfTracer::incr(PerfEventType::AM_OFFSET_CACHE_HIT, volId);
            LOGDEBUG << "Found offset, id: " << *obj_id;
            obj_ids.push_back(obj_id);
        } else {
            return err; // Had a cache miss, inform processor
        }
    }
    return ERR_OK;
}

std::pair<size_t, size_t>
AmCache::getObjects(fds_volid_t volId,
                 std::vector<ObjectID::ptr> const& objectIds,
                 std::vector<boost::shared_ptr<std::string>>& objects)
{
    static boost::shared_ptr<std::string> null_object = boost::make_shared<std::string>();
    fds_verify(objectIds.size() == objects.size());
    size_t hit_cnt = 0, miss_cnt = 0;
    auto id_it = objectIds.begin();
    auto data_it = objects.begin();
    for (; id_it != objectIds.end(); ++id_it, ++data_it) {
        auto const& obj_id = *id_it;
        boost::shared_ptr<std::string> blobObjectPtr = null_object;
        GLOGTRACE << "Cache lookup for volume " << std::hex << volId << std::dec
                  << " object " << *obj_id;

        // If this is a null object return a zero size object to the connector,
        if (NullObjectID != *obj_id) {
            auto err = object_cache.get(volId, *obj_id, blobObjectPtr);
            if (ERR_OK != err) {
                ++miss_cnt;
                continue;
            }
        }
        ++hit_cnt;
        PerfTracer::incr(PerfEventType::AM_OBJECT_CACHE_HIT, volId);
        *data_it = blobObjectPtr;
    }
    return std::make_pair(hit_cnt, miss_cnt);
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
    // If the transaction is a delete, we want to remove the cache entry
    if (txDesc->opType == FDS_DELETE_BLOB) {
        LOGTRACE << "Cache remove tx descriptor for volume "
                 << txDesc->volId << " blob " << txDesc->blobName;
        // Remove from blob caches
        removeBlob(txDesc->volId, txDesc->blobName);
    } else {
        fds_verify(txDesc->opType == FDS_PUT_BLOB);
        LOGTRACE << "Cache insert tx descriptor for volume "
                 << txDesc->volId << " blob " << txDesc->blobName;

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
        putBlobDescriptor(fds_volid_t(cacheDesc->getVolId()), cacheDesc->getBlobName(), cacheDesc);

        // Add blob objects from tx to object cache
        for (const auto &object : txDesc->stagedBlobObjects) {
            putObject(txDesc->volId, object.first, object.second);
        }
    }
    return ERR_OK;
}

Error
AmCache::removeBlob(fds_volid_t volId, const std::string &blobName) {
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
