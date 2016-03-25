/*
 * Copyright 2016 Formation Data Systems, Inc.
 */
#include "AmBlockLayer.h"

#include <fds_process.h>
#include "AmTxManager.h"
#include "connector/SectorLockMap.h"
#include <ObjectId.h>
#include "requests/GetBlobReq.h"
#include "requests/PutBlobReq.h"
#include "requests/PutObjectReq.h"
#include "requests/UpdateCatalogReq.h"

namespace fds {

BlockUpdate::BlockUpdate(offset_type const obj_off) :
    object_data(nullptr),
    object_id(),
    absolute_offset(obj_off),
    notify_set()
{ }

BlockUpdate::BlockUpdate(AmMultiReq* root,
                         offset_type const abs_off,
                         offset_type const obj_off,
                         data_buf_type buf,
                         bool const complete) :
    dirty(true),
    object_complete(complete),
    object_data(buf),
    object_id(),
    object_offset(obj_off),
    absolute_offset(abs_off),
    notify_set()
{
    if (root) notify_set.insert(root);
}

void
BlockUpdate::rehash() {
    if (!dirty || !object_data) return;
    object_length = object_data->length();
    if (0 == object_length) {
        object_id = NullObjectID;
    } else {
        object_id = ObjectID(ObjIdGen::genObjectId(object_data->c_str(), object_length));
    }
    dirty = false;
}

BlockUpdate&
BlockUpdate::operator>>(BlockUpdate& rhs) const
{
    for (auto& req : notify_set) {
        rhs.notify_set.insert(req);
    }
    return rhs;
}

BlockUpdate&
BlockUpdate::operator<<(BlockUpdate& rhs)
{ 
    // Just swap the buffer if this is a complete write
    if (object_data && !rhs.object_complete) {
        if (cached) {
            // Make a copy of the buffer to merge it may be living in cache too!
            object_data = boost::make_shared<data_type>(*object_data);
            cached = false;
        }
        object_data->replace(rhs.object_offset,
                             rhs.object_data->length(),
                             rhs.object_data->c_str(),
                             rhs.object_data->length());
    } else {
        object_data = rhs.object_data;
        cached = rhs.cached;
        object_offset = rhs.object_offset;
    }
    dirty = true;

    if (!notify_set.empty() && !rhs.notify_set.empty()) {
        (*notify_set.begin())->mergeDependencies(*rhs.notify_set.begin());
    }
    for (auto& req : rhs.notify_set) {
        notify_set.insert(req);
    }
    return *this;
}

bool
BlockUpdate::notifyAll(Error const& e) {
    for (auto req : notify_set) {
        req->notifyResponse(e, absolute_offset);
    }
    return (notify_set.empty() ? true
                                 : (*notify_set.begin())->dependenciesDone());
}

Error
BlockUpdate::result() const {
    return (notify_set.empty() ? ERR_OK : (*notify_set.begin())->errorCode());
}

AmBlockLayer::AmBlockLayer(AmDataProvider* prev) :
    AmDataProvider(prev, new AmTxManager(this)),
    block_queue()
{
}

AmBlockLayer::~AmBlockLayer() = default;

void
AmBlockLayer::putBlobOnce(AmRequest *amReq) {
    auto blobReq = static_cast<PutBlobReq*>(amReq);
    std::deque<BlockUpdate> needed_offsets;
    std::deque<BlockUpdate> ready_to_put;

    // Split and enqueue write. Vectors will be populated with requests that
    // *this* thread should enqueue.
    auto objects = split_update(blobReq, needed_offsets, ready_to_put);
    if (0 == objects) {
        auto aligned_off = (blobReq->blob_offset / blobReq->object_size) * blobReq->object_size;
        auto catReq = new UpdateCatalogReq(blobReq, true);
        catReq->metadata->insert(blobReq->metadata->begin(), blobReq->metadata->end());
        catReq->volInfoCopy(blobReq);
        AmDataProvider::updateCatalog(catReq);
        return;
    }
    dispatchReads(blobReq, needed_offsets);

    for (auto& update: ready_to_put) {
        dispatchPut(blobReq, update);
    }
}

void
AmBlockLayer::getBlobCb(AmRequest* amReq, Error const error) {
    auto blobReq = static_cast<GetBlobReq*>(amReq);
    auto err = error;

    // Here's the block map with pending requests
    auto offset_queue = lookup_offset_queue(blobReq->io_vol_id, blobReq->getBlobName());
    // TODO(bszmyd): Sat 19 Mar 2016 03:52:57 AM MDT
    // Don't call for every callback
    if (offset_queue) {
        BlockUpdate::data_buf_type empty_buffer;
        auto offset = blobReq->blob_offset;
        auto const& obj_size = blobReq->object_size;
        auto cb = std::dynamic_pointer_cast<GetObjectCallback>(blobReq->cb);
        auto buf_it = cb->return_buffers->begin();
        for (; blobReq->blob_offset_end >= offset; offset += obj_size) {
            BlockUpdate update { offset };
            if (ERR_OK == err || ERR_BLOB_NOT_FOUND == err) {
                BlockUpdate::data_buf_type buffer;
                if (cb->return_buffers->end() == buf_it
                    || !(*buf_it)
                    || 0 == (*buf_it)->size()) {
                    if (!empty_buffer) {
                        empty_buffer = boost::make_shared<std::string>(obj_size, '\0');
                    }
                    buffer = empty_buffer;
                } else {
                    // Copy the buffer so we don't corrupt the read cache
                    buffer = *buf_it;
                    ++buf_it;
                }
                update = BlockUpdate {nullptr, offset, 0, buffer};
                update.setCached();
                err = ERR_OK;
            }
           
            std::deque<BlockUpdate> need_dispatch;
            auto result = offset_queue->read_resp(update, need_dispatch, err);

            if (sector_type::queue_result_type::MergedEntry == result) {
                dispatchPut(blobReq, update);
            } else if (sector_type::queue_result_type::Finished == result) {
                for (auto req : update.request_set()) {
                    AmDataProvider::putBlobOnceCb(req, update.result());
                }
                dispatchReads(blobReq, need_dispatch);
            }
        }
    }
    AmDataProvider::getObjectCb(amReq, err);
}

void
AmBlockLayer::putObjectCb(AmRequest* amReq, Error const error) {
    auto blobReq = static_cast<PutObjectReq*>(amReq);

    // Here's the block map with pending requests
    auto offset_queue = lookup_offset_queue(blobReq->io_vol_id, blobReq->getBlobName());
    // TODO(bszmyd): Sat 19 Mar 2016 03:53:40 AM MDT
    // Don't call for every put
    if (offset_queue) {
        BlockUpdate update { blobReq->blob_offset };
        update.setId(blobReq->obj_id);
        std::deque<BlockUpdate> need_dispatch;
        auto result = offset_queue->write_resp(update, need_dispatch, error);
        if (sector_type::queue_result_type::MergedEntry == result) {
            if (ERR_OK == error) {
                auto putReq = static_cast<PutBlobReq*>(*need_dispatch.front().request_set().begin());
                auto catReq = new UpdateCatalogReq(putReq, true);
                for (auto part_update : need_dispatch) {
                    catReq->object_list.emplace(part_update.id(),
                                                std::make_pair(part_update.offset(),
                                                               part_update.objectLength()));
                }
                for (auto req : need_dispatch.front().request_set()) {
                    auto pReq = static_cast<PutBlobReq*>(req);
                    catReq->metadata->insert(pReq->metadata->begin(), pReq->metadata->end());
                }
                catReq->volInfoCopy(amReq);
                AmDataProvider::updateCatalog(catReq);
            } else {
                // We can retry with this rolled up object
                dispatchPut(blobReq, update);
            }
        } else if (sector_type::queue_result_type::Finished == result) {
            for (auto req : update.request_set()) {
                AmDataProvider::putBlobOnceCb(req, update.result());
            }
            dispatchReads(blobReq, need_dispatch);
        }
    }
    delete amReq;
}

void
AmBlockLayer::updateCatalogCb(AmRequest* amReq, Error const error)  {
    auto blobReq = static_cast<UpdateCatalogReq*>(amReq);
    auto err = error;

    if (!blobReq->object_list.empty()) {
        // Here's the block map with pending requests
        auto offset_queue = lookup_offset_queue(blobReq->io_vol_id, blobReq->getBlobName());
        // TODO(bszmyd): Sat 19 Mar 2016 03:53:40 AM MDT
        // Don't call for every put
        if (offset_queue) {
            // Build a matching Update for the sector lock to find
            ObjectID object_id;
            std::pair<uint64_t, size_t> object_pair;
            std::tie(object_id, object_pair) = *blobReq->object_list.begin();
            BlockUpdate update { nullptr, object_pair.first, 0, nullptr };
            update.setId(object_id);

            std::deque<BlockUpdate> need_dispatch;
            auto result = offset_queue->catalog_resp(update, need_dispatch, err);
            err = update.result();
            for (auto req : update.request_set()) {
                AmDataProvider::putBlobOnceCb(req, err);
            }
            // If the update to the offset failed, we'll get back some Gets that
            // need getting, otherwise we'll get back puts that need putting
            if (ERR_OK == err) {
                for (auto& needed : need_dispatch) {
                    dispatchPut(blobReq, needed);
                }
            } else {
                dispatchReads(blobReq, need_dispatch);
            }
        }
    } else {
        // This was an empty write, just respond to the parent
        if (blobReq->parent) {
            AmDataProvider::putBlobOnceCb(blobReq->parent, err);
        }
    }
    delete blobReq;
}

AmBlockLayer::offset_queue_ptr
AmBlockLayer::lookup_offset_queue(fds_volid_t const vol_id,
                                  std::string const& blob,
                                  bool const create_too) {
    std::lock_guard<std::mutex> g(block_update_lock);
    auto& blob_map = block_queue[vol_id];
    auto blob_it = blob_map.find(blob);
    if (blob_map.end() != blob_it || create_too) {
        if (blob_map.end() == blob_it) {
            bool happened {false};
            std::tie(blob_it, happened) = blob_map.emplace(blob,
                                                           std::make_shared<
                                                           sector_type>());
            fds_assert(happened);
        }
        return blob_it->second;
    }
    return nullptr;
}

size_t
AmBlockLayer::split_update(PutBlobReq* blobReq, std::deque<BlockUpdate>& needed, std::deque<BlockUpdate>& ready) {
    size_t data_written = 0;
    size_t objects = 0;
    auto const& bytes       = blobReq->dataPtr;
    auto const& length      = blobReq->data_len;
    auto const& obj_size    = blobReq->object_size;
    auto const& offset      = blobReq->blob_offset;
    auto const blob_name = blobReq->getBlobName();

    // While we have data to write, find the aligned block and create an update
    // for it, accumulate the data for that write and continue
    while (data_written < length) {
        auto const curr_offset = offset + data_written;     // Absolute offset
        auto const part_off = curr_offset % obj_size;       // Offset within block
        auto const aligned_off = curr_offset - part_off;    // Absolute offset of block
        auto const part_length = std::min((obj_size - part_off), // Data for this part
                                          (length - data_written));

        LOGTRACE  << "Will write offset: 0x" << std::hex << curr_offset
                  << " for length: 0x" << part_length;

        // Trim (if needed) the buffer to create an update solely for this block
        auto objBuf = (part_length == length) ?
            bytes : boost::make_shared<std::string>(*bytes, data_written, part_length);
        data_written += part_length;
        blobReq->expectResponseOn(aligned_off);
        ++objects;

        // Create a queue-map for this volume-blob nexus if needed
        auto offset_queue = lookup_offset_queue(blobReq->io_vol_id, blob_name, true);

        // Queue the update against the map
        auto complete = (blob::TRUNCATE == blobReq->blob_mode ? true : (obj_size == part_length));
        BlockUpdate update {blobReq, aligned_off, part_off, objBuf, complete};
        auto result = offset_queue->queue_update(update);
        if (sector_type::queue_result_type::MergedEntry != result) {
            // This is a RMW and we don't have the data already
            if (sector_type::queue_result_type::FirstEntry == result) {
                // We need to dispatch a get to DM/SM for this data
                needed.emplace_back(update);
            }
        } else {
            // This block is ready to be written to SM
            ready.emplace_back(update);
        }
    }
    return objects;
}

void
AmBlockLayer::dispatchPut(AmRequest* parent_request, BlockUpdate& need_dispatch) {
    auto putReq = new PutObjectReq(parent_request->io_vol_id,
                                   parent_request->volume_name,
                                   parent_request->getBlobName(),
                                   need_dispatch.data(),
                                   need_dispatch.id(),
                                   need_dispatch.objectLength());
    putReq->blob_offset = need_dispatch.offset();
    putReq->volInfoCopy(parent_request);
    AmDataProvider::putObject(putReq);
}

void
AmBlockLayer::dispatchReads(AmRequest* parent_request,
                            std::deque<BlockUpdate>& need_dispatch) {
    if (!need_dispatch.empty()) {
        auto begin_off = need_dispatch.front().offset();
        auto end_off = begin_off;
        for (auto dispatch : need_dispatch) {
            begin_off = std::min(begin_off, dispatch.offset());
            end_off = std::max(end_off, dispatch.offset());
        }
        auto get_length = (end_off - begin_off) + parent_request->object_size;
        auto closure = [](GetObjectCallback* cb, fpi::ErrorCode const& e) -> void { };
        auto callback = create_async_handler<GetObjectCallback>(std::move(closure));
        auto getReq = new GetBlobReq(parent_request->io_vol_id,
                                     parent_request->volume_name,
                                     parent_request->getBlobName(),
                                     callback,
                                     begin_off,
                                     get_length);
        getReq->volInfoCopy(parent_request);
        AmDataProvider::getBlob(getReq);
    }
}


}  // namespace fds
