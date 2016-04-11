/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <AmCache.h>

#include <climits>
#include <fds_process.h>
#include <PerfTrace.h>
#include <lib/StatsCollector.h>
#include "AmDispatcher.h"
#include "AmTxDescriptor.h"
#include "requests/AttachVolumeReq.h"
#include "requests/CommitBlobTxReq.h"
#include "requests/GetBlobReq.h"
#include "requests/GetObjectReq.h"
#include "requests/PutObjectReq.h"
#include "requests/RenameBlobReq.h"
#include "requests/UpdateCatalogReq.h"

namespace fds {

static constexpr fds_uint32_t Ki { 1024 };
static constexpr fds_uint32_t Mi { 1024 * Ki };

AmCache::AmCache(AmDataProvider* prev)
    : AmDataProvider(prev, new AmDispatcher(this)),
      max_metadata_entries(0)
{
    /**
     * FEATURE TOGGLE: Cache missing offsets and descriptors (nulls)
     * Sat Jan 30 11:36:31 2016
     */
    FdsConfigAccessor ft_conf(g_fdsprocess->get_fds_config(), "fds.feature_toggle.");
    cache_missing_cat = ft_conf.get<bool>("am.cache_missing_cat_entries", cache_missing_cat);

    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    max_metadata_entries = std::min((uint64_t)LLONG_MAX, (uint64_t)conf.get<int64_t>("cache.max_metadata_entries"));
    // This is in terms of MiB
    max_volume_data = Mi * conf.get<fds_uint32_t>("cache.max_volume_data");
}

AmCache::~AmCache() = default;

bool
AmCache::done() {
    {
        std::lock_guard<std::mutex> g(obj_get_lock);
        if (!obj_get_queue.empty()) {
            return false;
        }
    }
    return AmDataProvider::done();
}

BlobDescriptor::ptr
AmCache::getBlobDescriptor(AmRequest* amReq, Error &error) {
    auto const& volId = amReq->io_vol_id;
    auto const blobName = amReq->getBlobName();
    LOGTRACE << "volid:" << volId << " blob:" << blobName << " cache lookup";

    BlobDescriptor::ptr blobDescPtr;
    error = descriptor_cache.get(volId, blobName, blobDescPtr);
    if (error == ERR_OK) {
        PerfTracer::incr(PerfEventType::AM_DESC_CACHE_HIT, volId);
    }
    return blobDescPtr;
}

Error
AmCache::getBlobOffsetObjects(GetBlobReq* blobReq) {
    auto const& volId = blobReq->io_vol_id;

    // Search for all the Offset pairs for the range of data we're interested in
    // and populate the object ids vector with ids.
    blobReq->object_ids.clear();
    Error error {ERR_OK};
    for (auto cur_off = blobReq->blob_offset;
         blobReq->blob_offset_end >= cur_off;
         cur_off += blobReq->object_size) {
        ObjectID::ptr obj_id;
        auto offset_pair = BlobOffsetPair(blobReq->getBlobName(), cur_off);
        auto err = offset_cache.get(volId, offset_pair, obj_id);
        if (err == ERR_OK) {
            PerfTracer::incr(PerfEventType::AM_OFFSET_CACHE_HIT, volId);
            LOGDEBUG << "objid:" << *obj_id << " offset:" << cur_off << " cache hit";
            blobReq->object_ids.push_back(obj_id);
        } else {
            LOGDEBUG << "offset:" << cur_off << " cache miss";
            error = err;
            blobReq->object_ids.push_back(boost::make_shared<ObjectID>(NullObjectID));
        }
    }
    return error;
}

void
AmCache::getObjects(GetBlobReq* blobReq) {
    static boost::shared_ptr<std::string> null_object = boost::make_shared<std::string>(0, '\0');

    LOGDEBUG << "numobj:" << blobReq->object_ids.size() << " checking cache";
    auto cb = std::dynamic_pointer_cast<GetObjectCallback>(blobReq->cb);
    cb->return_buffers->assign(blobReq->object_ids.size(), nullptr);
    cb->return_buffers->shrink_to_fit();

    fds_verify(blobReq->object_ids.size() == cb->return_buffers->size());
    size_t hit_cnt = 0, miss_cnt = 0;
    auto id_it = blobReq->object_ids.begin();
    auto data_it = cb->return_buffers->begin();
    for (; id_it != blobReq->object_ids.end(); ++id_it, ++data_it) {
        auto const& obj_id = *id_it;
        boost::shared_ptr<std::string> blobObjectPtr = null_object;
        // If this is a null object we don't know or it doesn't have an ObjectID
        Error err {ERR_OK};
        if (NullObjectID != *obj_id) {
            LOGDEBUG << "volid:" << blobReq->io_vol_id << " objid:" << *obj_id << " cache lookup";
            err = object_cache.get(blobReq->io_vol_id, *obj_id, blobObjectPtr);
        }

        if (ERR_OK != err) {
            ++miss_cnt;
            continue;
        }

        ++hit_cnt;
        PerfTracer::incr(PerfEventType::AM_OBJECT_CACHE_HIT, blobReq->io_vol_id);

        auto io_done_ts = util::getTimeStampNanos();
        fds_uint64_t total_nano = io_done_ts - blobReq->enqueue_ts;

        auto io_total_time = static_cast<double>(total_nano) / 1000.0;

        StatsCollector::singleton()->recordEvent(blobReq->io_vol_id,
                                                 io_done_ts,
                                                 STAT_AM_GET_OBJ,
                                                 io_total_time);

        StatsCollector::singleton()->recordEvent(blobReq->io_vol_id,
                                                 io_done_ts,
                                                 STAT_AM_GET_CACHED_OBJ,
                                                 io_total_time);

        data_it->swap(blobObjectPtr);
    }

    if (0 == miss_cnt) {
        // Data was found in cache, done
        LOGTRACE << "data found in cache";
        getBlobCb(blobReq, ERR_OK);
        return;
    }

    // We did not find all the data, so create some GetObjectReqs and defer to
    // SM for the rest by iterating and checking if the buffer is valid, if
    // not use the object to create a new GetObjectReq
    LOGDEBUG << "objects:" << hit_cnt << "/" << (hit_cnt + miss_cnt) << " found";
    blobReq->setResponseCount(miss_cnt);
    auto obj_it = blobReq->object_ids.cbegin();
    auto buf_it = cb->return_buffers->begin();

    // Short-circuit this loop when we've dispatched all the expected requests,
    // otherwise we'll continue to use what might be pointers to a request that
    // has already been responded to on another thread.
    for (; 0 < miss_cnt; ++obj_it, ++buf_it) {
        if (!*buf_it) {
            --miss_cnt;
            LOGDEBUG << "obj:" << **obj_it << " instantiating GetObject";
            getObject(blobReq, *obj_it, *buf_it);
        }
    }
}

void
AmCache::getObject(GetBlobReq* blobReq,
                   ObjectID::ptr const& obj_id,
                   boost::shared_ptr<std::string>& buf) {
    auto objReq = new GetObjectReq(blobReq, buf, obj_id);
    bool happened {false};
    // Only dispatch to lower layers if we haven't already for this object
    {
        std::lock_guard<std::mutex> g(obj_get_lock);
        auto it = obj_get_queue.end();
        std::tie(it, happened) = obj_get_queue.emplace(*obj_id, nullptr);
        if (happened) {
            it->second.reset(new std::deque<GetObjectReq*>());
        }
        it->second->push_back(objReq);
    }
    if (happened) {
        AmDataProvider::getObject(objReq);
    }
}

void
AmCache::getObjectCb(AmRequest* amReq, Error const error) {
    auto const obj_id = *static_cast<GetObjectReq*>(amReq)->obj_id;
    std::unique_ptr<std::deque<GetObjectReq*>> queue;
    {
        // Find the waiting get request queue
        std::lock_guard<std::mutex> g(obj_get_lock);
        auto q = obj_get_queue.find(obj_id);
        if (obj_get_queue.end() != q) {
            queue.swap(q->second);
            obj_get_queue.erase(q);
        }
    }

    if (!queue) {
        LOGCRITICAL << "missing response queue, assuming lost getObject request? Possible Leak";
        return;
    }

    auto io_done_ts = util::getTimeStampNanos();
    // For each request waiting on this object, respond to it and set the buffer
    // data member from the first request (the one that was actually dispatched)
    // and populate the volume's cache
    auto buf = queue->front()->obj_data;
    for (auto objReq : *queue) {
        if (error.ok()) {
            objReq->obj_data = buf;
            object_cache.add_dirty(objReq->io_vol_id, obj_id, objReq->obj_data);
        }
        fds_uint64_t total_nano = io_done_ts - static_cast<GetObjectReq*>(objReq)->blobReq->enqueue_ts;

        bool done;
        Error err;
        std::tie(done, err) = objReq->blobReq->notifyResponse(error);
        if (done) {
            getBlobCb(objReq->blobReq, err);
        }


        auto io_total_time = static_cast<double>(total_nano) / 1000.0;

        StatsCollector::singleton()->recordEvent(objReq->io_vol_id,
                                                 io_done_ts,
                                                 STAT_AM_GET_OBJ,
                                                 io_total_time);

        delete objReq;
    }
}

Error
AmCache::putTxDescriptor(const std::shared_ptr<AmTxDescriptor> txDesc, fds_uint64_t const blobSize) {
    // If the transaction is a delete, we want to remove the cache entry
    if (txDesc->opType == FDS_DELETE_BLOB) {
        LOGTRACE << "volid:" << txDesc->volId << " blob:" << txDesc->blobName
                 << " cache remove tx descriptor";
        // Remove from blob caches
        if (cache_missing_cat) {
            /// FEATURE TOGGLE: cache missing entry
            descriptor_cache.add(txDesc->volId, txDesc->blobName, nullptr);
        } else {
            descriptor_cache.remove(txDesc->volId, txDesc->blobName);
        }
        offset_cache.remove_if(txDesc->volId,
                               [blobName = txDesc->blobName] (BlobOffsetPair const& blob_pair) -> bool {
                                   return (blobName == blob_pair.getName());
                               });
    } else {
        fds_verify(txDesc->opType == FDS_PUT_BLOB);
        LOGTRACE << "volid:" << txDesc->volId << " blob:" << txDesc->blobName
                 << "c cache insert";

        for (auto& offset_pair : txDesc->stagedBlobOffsets) {
            auto const& obj_id = offset_pair.second.first;
            offset_cache.add(txDesc->volId,
                             offset_pair.first,
                             boost::make_shared<ObjectID>(obj_id));
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
            LOGTRACE << "objid:" << object.first << " length:" << object.second->size()
                     << " cache insert object";
            object_cache.add(txDesc->volId, object.first, object.second);
        }
    }
    return ERR_OK;
}

void
AmCache::closeVolume(AmRequest *amReq) {
    descriptor_cache.removeVolume(amReq->io_vol_id);
    offset_cache.removeVolume(amReq->io_vol_id);
    object_cache.removeVolume(amReq->io_vol_id);
    AmDataProvider::closeVolume(amReq);
}

void
AmCache::statBlob(AmRequest *amReq) {
    // Can we read from the cache
    if (!amReq->forced_unit_access) {
        // Check cache for blob descriptor
        Error err(ERR_OK);
        BlobDescriptor::ptr cachedBlobDesc = getBlobDescriptor(amReq, err);

        if (ERR_OK == err) {
            // FEATURE TOGGLE: read missing cat entries from cache
            if (cache_missing_cat && !cachedBlobDesc) {
                LOGDEBUG << "blob:" << amReq->getBlobName() << " read missing blob descriptor";
                AmDataProvider::statBlobCb(amReq, ERR_BLOB_NOT_FOUND);
            } else {
                LOGTRACE << "blob:" << amReq->getBlobName() << " volid:" << amReq->io_vol_id
                         << " found cached blob descriptor";

                auto cb = std::dynamic_pointer_cast<StatBlobCallback>(amReq->cb);
                // Fill in the data here
                cb->blobDesc = cachedBlobDesc;
                AmDataProvider::statBlobCb(amReq, ERR_OK);
            }
            return;
        }
        LOGTRACE << "blob:" << amReq->getBlobName() << " volid:" << amReq->io_vol_id
                 << " did not find cached blob descriptor";
    }

    return AmDataProvider::statBlob(amReq);
}

void
AmCache::openVolumeCb(AmRequest* amReq, Error const error) {
    // Let's dump our meta cache to be safe if we loose a lease on a volume
    auto const& vol_uuid = amReq->io_vol_id;
    if (ERR_OK != error || !static_cast<AttachVolumeReq*>(amReq)->mode.can_cache) {
        descriptor_cache.clear(vol_uuid);
        offset_cache.clear(vol_uuid);
        if (ERR_OK == error) {
            object_cache.addVolume(vol_uuid, max_volume_data);
        }
    } else if (ERR_VOL_DUPLICATE != descriptor_cache.addVolume(vol_uuid, max_metadata_entries)) {
        offset_cache.addVolume(vol_uuid, max_metadata_entries);
        object_cache.addVolume(vol_uuid, max_volume_data);
        LOGDEBUG << "Created caches for volume: " << amReq->volume_name;
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
                             std::dynamic_pointer_cast<RenameBlobCallback>(amReq->cb)->blobDesc);
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
        descriptor_cache.add_dirty(amReq->io_vol_id,
                                   amReq->getBlobName(),
                                   std::dynamic_pointer_cast<StatBlobCallback>(amReq->cb)->blobDesc);
    } else if (cache_missing_cat && ERR_BLOB_NOT_FOUND == error) {
        /// FEATURE TOGGLE: Cache missing entry
        descriptor_cache.add_dirty(amReq->io_vol_id, amReq->getBlobName(), nullptr);
    }
    AmDataProvider::statBlobCb(amReq, error);
}

void
AmCache::getBlob(AmRequest *amReq) {
    GetBlobReq *blobReq = static_cast<GetBlobReq *>(amReq);

    // Can we read from cache
    if (!blobReq->forced_unit_access) {
        // Check cache for descriptor on blob
        Error error { ERR_OK };
        blobReq->blobDesc = getBlobDescriptor(blobReq, error);
        if (error.ok()) {
            // FEATURE TOGGLE: read missing cat entries from cache
            blobReq->metadata_cached = true;
            if (cache_missing_cat) {
                if (!blobReq->blobDesc) {
                    LOGDEBUG << "blob:" << blobReq->getBlobName() << " read for missing blob";
                    getBlobCb(blobReq, ERR_BLOB_NOT_FOUND);
                    return;
                } else if (blobReq->blob_offset >= blobReq->blobDesc->getBlobSize()) {
                    LOGDEBUG << "blob:" << *blobReq->blobDesc << " offset:" << blobReq->blob_offset
                             << " read offset out of blob size";
                    getBlobCb(blobReq, ERR_OK);
                    return;
                }
            }
            LOGDEBUG << "blob:" << blobReq->getBlobName() << " volid:" << blobReq->io_vol_id
                     << " found cached blob descriptor";
        }

        // Check cache for object IDs
        error = getBlobOffsetObjects(blobReq);

        // ObjectIDs were found in the cache
        if (error.ok() && (blobReq->metadata_cached || !blobReq->get_metadata)) {
            // Found all metadata, just need object data
            blobReq->metadata_cached = true;
            getObjects(blobReq);
            return;
        }
    } else {
        LOGDEBUG << "can't read from cache, dispatching to DM";
        size_t numObjs = ((blobReq->blob_offset_end - blobReq->blob_offset) / blobReq->object_size) + 1;
        blobReq->object_ids.assign(numObjs, boost::make_shared<ObjectID>(NullObjectID));
    }

    blobReq->metadata_cached = false;
    AmDataProvider::getOffsets(blobReq);
}

void
AmCache::getOffsetsCb(AmRequest* amReq, Error const error) {
    // If we got the data successfully
    auto blobReq = static_cast<GetBlobReq*>(amReq);
    if (ERR_OK == error) {
        return getObjects(blobReq);
    }
    getBlobCb(blobReq, error);
}

void
AmCache::getBlobCb(AmRequest *amReq, Error const error) {
    auto blobReq = static_cast<GetBlobReq *>(amReq);
    if (ERR_OK == error) {
        // We still return all of the object even if less was requested, adjust
        // the return length, not the buffer.
        auto cb = std::dynamic_pointer_cast<GetObjectCallback>(blobReq->cb);

        // Calculate the sum size of our buffers
        size_t vector_size = std::accumulate(cb->return_buffers->cbegin(),
                                             cb->return_buffers->cend(),
                                             0,
                                             [] (size_t const& total_size, boost::shared_ptr<std::string> const& buf)
                                             { return total_size + buf->size(); });

        cb->return_size = std::min(blobReq->data_len, vector_size);
    }

    // If we have a cache token, we can stash this metadata
    if (blobReq->blobDesc) {
        if (!blobReq->metadata_cached && blobReq->page_out_cache) {
            descriptor_cache.add_dirty(blobReq->io_vol_id,
                                       blobReq->getBlobName(),
                                       blobReq->blobDesc);
            auto iOff = blobReq->blob_offset;
            for (auto const& obj_id : blobReq->object_ids) {
                auto blob_pair = BlobOffsetPair(blobReq->getBlobName(), iOff);
                offset_cache.add_dirty(blobReq->io_vol_id, blob_pair, obj_id);
                iOff += blobReq->object_size;
            }
        }
        if (blobReq->get_metadata) {
            auto cbm = std::dynamic_pointer_cast<GetObjectWithMetadataCallback>(blobReq->cb);
            cbm->blobDesc = blobReq->blobDesc;
        }
    } else if (!blobReq->metadata_cached
               && cache_missing_cat
               && ERR_BLOB_NOT_FOUND == error) {
        /// FEATURE TOGGLE: Cache missing entry
        descriptor_cache.add_dirty(blobReq->io_vol_id, blobReq->getBlobName(), nullptr);
    }

    AmDataProvider::getBlobCb(blobReq, error);
}

void
AmCache::putObjectCb(AmRequest * amReq, Error const error) {
    if (error.ok()) {
        auto objReq = static_cast<PutObjectReq*>(amReq);
        object_cache.add(objReq->io_vol_id, objReq->obj_id, objReq->dataPtr);
    }
    AmDataProvider::putObjectCb(amReq, error);
}

void
AmCache::updateCatalogCb(AmRequest * amReq, Error const error) {
    auto blobReq = static_cast<UpdateCatalogReq*>(amReq);
    // If this was a PutBlobOnce we can stash the metadata changes
    if (error.ok() && fds::FDS_PUT_BLOB != blobReq->parent->io_type) {
        for (auto const& obj_upd : blobReq->object_list) {
            offset_cache.add(blobReq->io_vol_id,
                             BlobOffsetPair(blobReq->getBlobName(), obj_upd.second.first),
                             boost::make_shared<ObjectID>(obj_upd.first));
        }
        auto blobDesc = boost::make_shared<BlobDescriptor>(blobReq->getBlobName(),
                                                           blobReq->io_vol_id.get(),
                                                           blobReq->final_blob_size,
                                                           blobReq->final_meta_data);
        descriptor_cache.add(blobReq->io_vol_id, blobReq->getBlobName(), blobDesc);
    }
    AmDataProvider::updateCatalogCb(amReq, error);
}

}  // namespace fds
