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
#include "requests/GetBlobReq.h"
#include "requests/GetObjectReq.h"
#include "requests/RenameBlobReq.h"

namespace fds {

AmCache::AmCache(CommonModuleProviderIf *modProvider)
    : max_metadata_entries(0),
      dispatcher(new AmDispatcher(modProvider))
{
    FdsConfigAccessor conf(g_fdsprocess->get_fds_config(), "fds.am.");
    max_metadata_entries = std::min((uint64_t)LLONG_MAX, (uint64_t)conf.get<int64_t>("cache.max_metadata_entries"));
}

AmCache::~AmCache() = default;

void
AmCache::init(processor_cb_type cb) {
    processor_cb = cb;
    dispatcher->start();
}

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
            try {
            auto err = object_cache.get(blobReq->io_vol_id, *obj_id, blobObjectPtr);
            if (ERR_OK != err) {
                ++miss_cnt;
                continue;
            }
            } catch (std::exception &e) {
                fds_panic("%s", e.what());
            }
        }
        ++hit_cnt;
        PerfTracer::incr(PerfEventType::AM_OBJECT_CACHE_HIT, blobReq->io_vol_id);
        data_it->swap(blobObjectPtr);
    }

    if (0 == miss_cnt) {
        // Data was found in cache, done
        LOGTRACE << "Data found in cache!";
        return blobReq->proc_cb(ERR_OK);
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
    objReq->proc_cb = [this, objId = *obj_id] (Error const& error) {
                        this->getObjectCb(objId, error);
                      };

    std::lock_guard<std::mutex> g(obj_get_lock);
    auto q = obj_get_queue.find(*obj_id);
    if (obj_get_queue.end() == q) {
        dispatcher->getObject(objReq);
        auto new_queue = new std::deque<GetObjectReq*>();
        obj_get_queue[*obj_id].reset(new_queue);
        new_queue->push_back(objReq);
    } else {
        q->second->push_back(objReq);
    }
}

void
AmCache::getObjectCb(ObjectID const obj_id, Error const& error) {
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
        objReq->blobReq->notifyResponse(error);
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
        removeBlob(txDesc->volId, txDesc->blobName);
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

Error
AmCache::attachVolume(std::string const& volume_name) {
    return dispatcher->attachVolume(volume_name);
}

void
AmCache::openVolume(AmRequest *amReq) {
    return dispatcher->openVolume(amReq);
}

Error
AmCache::closeVolume(fds_volid_t vol_id, fds_int64_t token) {
    return dispatcher->closeVolume(vol_id, token);
}

void
AmCache::statVolume(AmRequest *amReq) {
    return dispatcher->statVolume(amReq);
}

void
AmCache::setVolumeMetadata(AmRequest *amReq) {
    return dispatcher->setVolumeMetadata(amReq);
}

void
AmCache::getVolumeMetadata(AmRequest *amReq) {
    return dispatcher->getVolumeMetadata(amReq);
}

void
AmCache::volumeContents(AmRequest *amReq) {
    return dispatcher->volumeContents(amReq);
}

void
AmCache::startBlobTx(AmRequest *amReq) {
    return dispatcher->startBlobTx(amReq);
}

void
AmCache::commitBlobTx(AmRequest *amReq) {
    return dispatcher->commitBlobTx(amReq);
}

void
AmCache::abortBlobTx(AmRequest *amReq) {
    return dispatcher->abortBlobTx(amReq);
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
            return processor_cb(amReq, ERR_OK);
        }
        LOGTRACE << "Did not find cached blob descriptor for " << std::hex
            << amReq->io_vol_id << std::dec << " blob " << amReq->getBlobName();
    }

    // Upon return, insert into cache if success
    amReq->proc_cb = [this, amReq] (Error const& error) mutable -> void {
        // Insert metadata into cache.
        if (ERR_OK == error && amReq->page_out_cache) {
            descriptor_cache.add(amReq->io_vol_id,
                                 amReq->getBlobName(),
                                 SHARED_DYN_CAST(StatBlobCallback, amReq->cb)->blobDesc);
        }
        processor_cb(amReq, error);
    };
    return dispatcher->statBlob(amReq);
}

void
AmCache::setBlobMetadata(AmRequest *amReq) {
    return dispatcher->setBlobMetadata(amReq);
}

void
AmCache::deleteBlob(AmRequest *amReq) {
    return dispatcher->deleteBlob(amReq);
}

void
AmCache::renameBlob(AmRequest *amReq) {
    amReq->proc_cb = [this, amReq] (Error const& error) mutable -> void {
        if (error.ok()) {
            // Place the new blob's descriptor in the cache
            auto blobReq = static_cast<RenameBlobReq*>(amReq);
            // Remove all offsets for the old blob and new blobs
            offset_cache.remove_if(amReq->io_vol_id,
                                   [blobReq] (BlobOffsetPair const& blob_pair) -> bool {
                                       return (blobReq->new_blob_name == blob_pair.getName() ||
                                               blobReq->getBlobName() == blob_pair.getName());
                                    });
            descriptor_cache.remove(amReq->io_vol_id, blobReq->getBlobName());
            descriptor_cache.add(blobReq->io_vol_id,
                                 blobReq->new_blob_name,
                                 SHARED_DYN_CAST(RenameBlobCallback, blobReq->cb)->blobDesc);
        }
        processor_cb(amReq, error);
    };
    return dispatcher->renameBlob(amReq);
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
            return getOffsetsCb(blobReq, error);
        }
    } else {
        LOGDEBUG << "Can't read from cache, dispatching to DM.";
    }

    amReq->proc_cb = [this, blobReq] (Error const& error) mutable -> void {
        getOffsetsCb(blobReq, error);
    };
    return dispatcher->getBlob(amReq);
}

void
AmCache::getOffsetsCb(GetBlobReq* blobReq, Error const& error) {
    // If we got the data successfully
    if (ERR_OK == error) {
        blobReq->proc_cb = [this, blobReq] (Error const& error) mutable -> void {
            getBlobCb(blobReq, error);
        };
        return getObjects(blobReq);
    }
    // Otherwise call callback now
    getBlobCb(blobReq, error);
}

void
AmCache::getBlobCb(AmRequest *amReq, const Error& error) {
    auto blobReq = static_cast<GetBlobReq *>(amReq);
    if (ERR_NOT_FOUND == error && !blobReq->retry) {
        // TODO(bszmyd): Mon 23 Mar 2015 02:40:25 AM PDT
        // This is somewhat of a trick since we don't really support
        // transactions. If we can't find the object, do an index lookup
        // again. If we get back the same ObjectID, then fine...try SM again,
        // but that's the end of it.
        blobReq->metadata_cached = false;
        blobReq->retry = true;
        GLOGDEBUG << "Dispatching retry on [ " << blobReq->volume_name
                  << ", " << blobReq->getBlobName()
                  << ", 0x" << std::hex << blobReq->blob_offset << std::dec
                  << "B ]";
        return getBlob(amReq);
    } else if (ERR_OK == error) {

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
    }

    processor_cb(amReq, error);
}


void
AmCache::putObject(AmRequest *amReq) {
    return dispatcher->putObject(amReq);
}

void
AmCache::putBlob(AmRequest *amReq) {
    return dispatcher->putBlob(amReq);
}

void
AmCache::putBlobOnce(AmRequest *amReq) {
    return dispatcher->putBlobOnce(amReq);
}

bool
AmCache::getNoNetwork() const {
    return dispatcher->getNoNetwork();
}

Error
AmCache::updateDlt(bool dlt_type, std::string& dlt_data, FDS_Table::callback_type const& cb) {
    return dispatcher->updateDlt(dlt_type, dlt_data, cb);
}

Error
AmCache::updateDmt(bool dmt_type, std::string& dmt_data, FDS_Table::callback_type const& cb) {
    return dispatcher->updateDmt(dmt_type, dmt_data, cb);
}

Error
AmCache::getDMT() {
    return dispatcher->getDMT();
}

Error
AmCache::getDLT() {
    return dispatcher->getDLT();
}

}  // namespace fds
