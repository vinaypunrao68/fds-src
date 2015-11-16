/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <AmCache.h>
#include <fds_process.h>
#include <PerfTrace.h>
#include <climits>
#include "AmDispatcher.h"
#include "AmTxDescriptor.h"
#include "requests/AttachVolumeReq.h"
#include "requests/GetBlobReq.h"
#include "requests/GetObjectReq.h"
#include "requests/RenameBlobReq.h"

namespace fds {

static constexpr fds_uint32_t Ki { 1024 };
static constexpr fds_uint32_t Mi { 1024 * Ki };

AmCache::AmCache(AmDataProvider* prev, CommonModuleProviderIf *modProvider)
    : AmDataProvider(prev, new AmDispatcher(this, modProvider)),
      max_metadata_entries(0)
{
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    max_metadata_entries = std::min((uint64_t)LLONG_MAX, (uint64_t)conf.get<int64_t>("cache.max_metadata_entries"));
    // This is in terms of MiB
    max_volume_data = Mi * conf.get<fds_uint32_t>("cache.max_volume_data");
}

AmCache::~AmCache() = default;

void
AmCache::start() {
    AmDataProvider::start();
}

void
AmCache::registerVolume(const VolumeDesc& volDesc) {
    auto const& vol_uuid = volDesc.volUUID;
    auto num_objs = (0 < volDesc.maxObjSizeInBytes) ?
        (max_volume_data / volDesc.maxObjSizeInBytes) : 0;
    descriptor_cache.addVolume(vol_uuid, max_metadata_entries);
    offset_cache.addVolume(vol_uuid, max_metadata_entries);
    object_cache.addVolume(vol_uuid, num_objs);
    LOGDEBUG << "Created caches for volume: " << volDesc.name;
    AmDataProvider::registerVolume(volDesc);
}

Error
AmCache::removeVolume(VolumeDesc const& volDesc) {
    descriptor_cache.removeVolume(volDesc.volUUID);
    offset_cache.removeVolume(volDesc.volUUID);
    object_cache.removeVolume(volDesc.volUUID);
    return AmDataProvider::removeVolume(volDesc);
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

void
AmCache::getObjects(GetBlobReq* blobReq) {
    static boost::shared_ptr<std::string> null_object = boost::make_shared<std::string>();

    LOGDEBUG << "checking cache for: " << blobReq->object_ids.size() << " objects";
    GetObjectCallback::ptr cb = SHARED_DYN_CAST(GetObjectCallback, blobReq->cb);
    cb->return_buffers->assign(blobReq->object_ids.size(), nullptr);
    cb->return_buffers->shrink_to_fit();

    fds_verify(blobReq->object_ids.size() == cb->return_buffers->size());
    size_t hit_cnt = 0, miss_cnt = 0;
    auto id_it = blobReq->object_ids.begin();
    auto data_it = cb->return_buffers->begin();
    for (; id_it != blobReq->object_ids.end(); ++id_it, ++data_it) {
        auto const& obj_id = *id_it;
        boost::shared_ptr<std::string> blobObjectPtr = null_object;
        GLOGTRACE << "Cache lookup for volume " << blobReq->io_vol_id << std::dec
                  << " object " << *obj_id;

        // If this is a null object return a zero size object to the connector,
        if (NullObjectID != *obj_id) {
            auto err = object_cache.get(blobReq->io_vol_id, *obj_id, blobObjectPtr);
            if (ERR_OK != err) {
                ++miss_cnt;
                continue;
            }
        }
        ++hit_cnt;
        PerfTracer::incr(PerfEventType::AM_OBJECT_CACHE_HIT, blobReq->io_vol_id);
        data_it->swap(blobObjectPtr);
    }

    if (0 == miss_cnt) {
        // Data was found in cache, done
        LOGTRACE << "Data found in cache!";
        return getBlobCb(blobReq, ERR_OK);
    }

    // We did not find all the data, so create some GetObjectReqs and defer to
    // SM for the rest by iterating and checking if the buffer is valid, if
    // not use the object to create a new GetObjectReq
    LOGDEBUG << "Found: " << hit_cnt << "/" << (hit_cnt + miss_cnt) << " objects";
    blobReq->setResponseCount(miss_cnt);
    auto obj_it = blobReq->object_ids.cbegin();
    auto buf_it = cb->return_buffers->begin();
    for (auto end = cb->return_buffers->cend(); end != buf_it; ++obj_it, ++buf_it) {
        if (!*buf_it) {
            LOGDEBUG << "Instantiating GetObject for object: " << **obj_it;
            getObject(blobReq, *obj_it, *buf_it);
        }
    }
}

void
AmCache::getObject(GetBlobReq* blobReq,
                   ObjectID::ptr const& obj_id,
                   boost::shared_ptr<std::string>& buf) {
    auto objReq = new GetObjectReq(blobReq, buf, obj_id);

    std::lock_guard<std::mutex> g(obj_get_lock);
    auto q = obj_get_queue.find(*obj_id);
    if (obj_get_queue.end() == q) {
        auto new_queue = new std::deque<GetObjectReq*>();
        obj_get_queue[*obj_id].reset(new_queue);
        new_queue->push_back(objReq);
        return AmDataProvider::getObject(objReq);
    }
    q->second->push_back(objReq);
}

void
AmCache::getObjectCb(AmRequest* amReq, Error const error) {
    auto obj_id = *static_cast<GetObjectReq*>(amReq)->obj_id;
    std::unique_ptr<std::deque<GetObjectReq*>> queue;
    {
        // Find the waiting get requeust queue
        std::lock_guard<std::mutex> g(obj_get_lock);
        auto q = obj_get_queue.find(obj_id);
        fds_assert(obj_get_queue.end() != q);
        queue.swap(q->second);
        obj_get_queue.erase(q);
    }

    // For each request waiting on this object, respond to it and set the buffer
    // data member from the first request (the one that was actually dispatched)
    // and populate the volume's cache
    auto buf = queue->front()->obj_data;
    for (auto& objReq : *queue) {
        if (error.ok()) {
            objReq->obj_data = buf;
            object_cache.add(objReq->io_vol_id, *objReq->obj_id, objReq->obj_data);
        }
        bool done;
        Error err;
        std::tie(done, err) = objReq->blobReq->notifyResponse(error);
        if (done) {
            getBlobCb(objReq->blobReq, err);
        }
        delete objReq;
    }
}

Error
AmCache::putTxDescriptor(const std::shared_ptr<AmTxDescriptor> txDesc, fds_uint64_t const blobSize) {
    // If the transaction is a delete, we want to remove the cache entry
    if (txDesc->opType == FDS_DELETE_BLOB) {
        LOGTRACE << "Cache remove tx descriptor for volume "
                 << txDesc->volId << " blob " << txDesc->blobName;
        // Remove from blob caches
        offset_cache.remove_if(txDesc->volId,
                               [blobName = txDesc->blobName] (BlobOffsetPair const& blob_pair) -> bool {
                                   return (blobName == blob_pair.getName());
                               });
        descriptor_cache.remove(txDesc->volId, txDesc->blobName);
    } else {
        fds_verify(txDesc->opType == FDS_PUT_BLOB);
        LOGTRACE << "Cache insert tx descriptor for volume "
                 << txDesc->volId << " blob " << txDesc->blobName;

        for (auto& offset_pair : txDesc->stagedBlobOffsets) {
            offset_cache.add(txDesc->volId,
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
        descriptor_cache.add(fds_volid_t(cacheDesc->getVolId()), cacheDesc->getBlobName(), cacheDesc);

        // Add blob objects from tx to object cache
        for (const auto &object : txDesc->stagedBlobObjects) {
            LOGTRACE << "Cache insert object data for " << object.first;
            LOGTRACE << "Cache insert object data length " << object.second->size();
            object_cache.add(txDesc->volId, object.first, object.second);
        }
    }
    return ERR_OK;
}

void
AmCache::statBlob(AmRequest *amReq) {
    // Can we read from the cache
    if (!amReq->forced_unit_access) {
        // Check cache for blob descriptor
        Error err(ERR_OK);
        BlobDescriptor::ptr cachedBlobDesc = getBlobDescriptor(amReq->io_vol_id,
                                                               amReq->getBlobName(),
                                                               err);
        if (ERR_OK == err) {
            LOGTRACE << "Found cached blob descriptor for " << std::hex
                << amReq->io_vol_id << std::dec << " blob " << amReq->getBlobName();

            StatBlobCallback::ptr cb = SHARED_DYN_CAST(StatBlobCallback, amReq->cb);
            // Fill in the data here
            cb->blobDesc = cachedBlobDesc;
            return AmDataProvider::statBlobCb(amReq, ERR_OK);
        }
        LOGTRACE << "Did not find cached blob descriptor for " << std::hex
            << amReq->io_vol_id << std::dec << " blob " << amReq->getBlobName();
    }

    return AmDataProvider::statBlob(amReq);
}

void
AmCache::deleteBlobCb(AmRequest* amReq, Error const error) {
    if (error.ok()) {
        descriptor_cache.remove(amReq->io_vol_id, amReq->getBlobName());
        offset_cache.remove_if(amReq->io_vol_id,
                               [amReq] (BlobOffsetPair const& blob_pair) -> bool {
                                    return (amReq->getBlobName() == blob_pair.getName());
                               });
    }
    AmDataProvider::deleteBlobCb(amReq, error);
}

void
AmCache::openVolumeCb(AmRequest* amReq, Error const error) {
    // Let's dump our meta cache to be safe if we loose a lease on a volume
    if (ERR_OK != error || !static_cast<AttachVolumeReq*>(amReq)->mode.can_cache) {
        descriptor_cache.clear(amReq->io_vol_id);
        offset_cache.clear(amReq->io_vol_id);
    }
    AmDataProvider::openVolumeCb(amReq, error);
}

void
AmCache::renameBlobCb(AmRequest* amReq, Error const error) {
    if (error.ok()) {
        // Place the new blob's descriptor in the cache
        auto blobReq = static_cast<RenameBlobReq*>(amReq);
        descriptor_cache.remove(amReq->io_vol_id, blobReq->getBlobName());
        descriptor_cache.add(blobReq->io_vol_id,
                             blobReq->new_blob_name,
                             SHARED_DYN_CAST(RenameBlobCallback, blobReq->cb)->blobDesc);
        // Remove all offsets for the old blob and new blobs
        offset_cache.remove_if(amReq->io_vol_id,
                               [blobReq] (BlobOffsetPair const& blob_pair) -> bool {
                                   return (blobReq->new_blob_name == blob_pair.getName() ||
                                           blobReq->getBlobName() == blob_pair.getName());
                                });
        // TODO(bszmyd): Sat 14 Nov 2015 02:39:16 AM MST
        // Should we move the offsets to the other blob so we don't have to
        // read them back post-rename?
    }
    AmDataProvider::renameBlobCb(amReq, error);
}

void
AmCache::statBlobCb(AmRequest* amReq, Error const error) {
    // Insert metadata into cache.
    if (ERR_OK == error && amReq->page_out_cache) {
        descriptor_cache.add(amReq->io_vol_id,
                             amReq->getBlobName(),
                             SHARED_DYN_CAST(StatBlobCallback, amReq->cb)->blobDesc);
    }
    AmDataProvider::statBlobCb(amReq, error);
}

void
AmCache::volumeContentsCb(AmRequest* amReq, Error const error) {
    if (ERR_OK == error && amReq->page_out_cache) {
        auto cb = SHARED_DYN_CAST(GetBucketCallback, amReq->cb);
        for (auto const& blob : *cb->vecBlobs) {
            descriptor_cache.add(amReq->io_vol_id,
                                 blob.getBlobName(),
                                 boost::make_shared<BlobDescriptor>(blob));
        }
    }
    AmDataProvider::volumeContentsCb(amReq, error);
}

void
AmCache::getBlob(AmRequest *amReq) {
    GetBlobReq *blobReq = static_cast<GetBlobReq *>(amReq);

    // Can we read from cache
    if (!amReq->forced_unit_access) {
        Error error {ERR_OK};
        // If we need to return metadata, check the cache
        if (blobReq->get_metadata) {
            auto cachedBlobDesc = getBlobDescriptor(amReq->io_vol_id,
                                                    amReq->getBlobName(),
                                                    error);
            if (error.ok()) {
                LOGTRACE << "Found cached blob descriptor for " << std::hex
                         << amReq->io_vol_id << std::dec << " blob " << amReq->getBlobName();
                blobReq->metadata_cached = true;
                auto cb = SHARED_DYN_CAST(GetObjectWithMetadataCallback, amReq->cb);
                // Fill in the data here
                cb->blobDesc = cachedBlobDesc;
            }
        }

        // Check cache for object IDs
        if (error.ok()) {
            error = getBlobOffsetObjects(amReq->io_vol_id,
                                         amReq->getBlobName(),
                                         amReq->blob_offset,
                                         amReq->blob_offset_end,
                                         amReq->object_size,
                                         blobReq->object_ids);
        }

        // ObjectIDs were found in the cache
        if (error.ok() && (blobReq->metadata_cached == blobReq->get_metadata)) {
            // Found all metadata, just need object data
            blobReq->metadata_cached = true;
            return getObjects(blobReq);
        }
    } else {
        LOGDEBUG << "Can't read from cache, dispatching to DM.";
    }

    AmDataProvider::getOffsets(amReq);
}

void
AmCache::getOffsetsCb(AmRequest* amReq, Error const error) {
    // If we got the data successfully
    auto blobReq = static_cast<GetBlobReq*>(amReq);
    if (ERR_OK == error) {
        return getObjects(blobReq);
    }
    // Otherwise call callback now
    AmDataProvider::getBlobCb(blobReq, error);
}

void
AmCache::getBlobCb(AmRequest *amReq, Error const error) {
    if (ERR_OK != error) {
        return AmDataProvider::getBlobCb(amReq, ERR_OK);
    }

    auto blobReq = static_cast<GetBlobReq *>(amReq);

    // We still return all of the object even if less was requested, adjust
    // the return length, not the buffer.
    GetObjectCallback::ptr cb = SHARED_DYN_CAST(GetObjectCallback, amReq->cb);

    // Calculate the sum size of our buffers
    size_t vector_size = std::accumulate(cb->return_buffers->cbegin(),
                                         cb->return_buffers->cend(),
                                         0,
                                         [] (size_t const& total_size, boost::shared_ptr<std::string> const& buf)
                                         { return total_size + buf->size(); });

    cb->return_size = std::min(amReq->data_len, vector_size);

    // If we have a cache token, we can stash this metadata
    if (!blobReq->metadata_cached && blobReq->page_out_cache) {
        auto iOff = amReq->blob_offset;
        for (auto const& obj_id : blobReq->object_ids) {
            auto blob_pair = BlobOffsetPair(amReq->getBlobName(), iOff);
            offset_cache.add(amReq->io_vol_id, blob_pair, obj_id);
            iOff += amReq->object_size;
        }
        if (blobReq->get_metadata) {
            auto cbm = SHARED_DYN_CAST(GetObjectWithMetadataCallback, amReq->cb);
            if (cbm->blobDesc)
                descriptor_cache.add(amReq->io_vol_id,
                                     amReq->getBlobName(),
                                     cbm->blobDesc);
        }
    }

    AmDataProvider::getBlobCb(amReq, ERR_OK);
}

}  // namespace fds
