/*
 * Copyright 2016 Formation Data Systems, Inc.
 */
#include "AmBlockLayer.h"

#include <fds_process.h>
#include "AmTxManager.h"
#include "requests/GetBlobReq.h"
#include "requests/PutBlobReq.h"
#include "requests/PutObjectReq.h"
#include "requests/UpdateCatalogReq.h"

namespace fds {

AmBlockLayer::AmBlockLayer(AmDataProvider* prev) :
    AmDataProvider(prev, new AmTxManager(this)),
    block_queue()
{
}

AmBlockLayer::~AmBlockLayer() = default;

void
AmBlockLayer::putBlobOnce(AmRequest *amReq) {
    auto blobReq = static_cast<PutBlobReq*>(amReq);
    std::deque<sector_type> needed_offsets;

    // Split and enqueue write. Vectors will be populated with requests that
    // *this* thread should enqueue.
    auto objects = split_update(blobReq, needed_offsets);
    if (0 == objects) {
        auto aligned_off = (blobReq->blob_offset / blobReq->object_size) * blobReq->object_size;
        auto catReq = new UpdateCatalogReq(blobReq, true);
        catReq->metadata->insert(blobReq->metadata->begin(), blobReq->metadata->end());
        catReq->volInfoCopy(blobReq);
        AmDataProvider::updateCatalog(catReq);
        return;
    }
    dispatchReads(blobReq, needed_offsets);
}

void
AmBlockLayer::getBlobCb(AmRequest* amReq, Error const error) {
    auto blobReq = static_cast<GetBlobReq*>(amReq);
    auto err = error;

    if (blobReq->for_rmw) {
        // Here's the block map with pending requests
        auto offset_queue = lookup_offset_queue(blobReq->io_vol_id, blobReq->getBlobName());
        if (offset_queue) {
            sector_type::data_buf_type empty_buffer;
            auto offset = blobReq->blob_offset;
            auto const& obj_size = blobReq->object_size;
            auto cb = std::dynamic_pointer_cast<GetObjectCallback>(blobReq->cb);
            auto buf_it = cb->return_buffers->begin();
            for (; blobReq->blob_offset_end >= offset; offset += obj_size) {
                sector_type update { offset };
                if (ERR_OK == err || ERR_BLOB_NOT_FOUND == err) {
                    sector_type::data_buf_type buffer;
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
                    update = sector_type {nullptr, offset, 0, buffer};
                    update.setCached();
                    err = ERR_OK;
                }

                offset_queue->read_resp(update, err);
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
    if (offset_queue) {
        sector_type update { blobReq->blob_offset };
        update.setId(blobReq->obj_id);
        offset_queue->write_resp(update, error);
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
        if (offset_queue) {
            // Build a matching Update for the sector lock to find
            ObjectID object_id;
            std::pair<uint64_t, size_t> object_pair;
            std::tie(object_id, object_pair) = *blobReq->object_list.begin();
            sector_type update { nullptr, object_pair.first, 0, nullptr };
            update.setId(object_id);

            offset_queue->catalog_resp(update, err);
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
                                                           offset_queue_type>(this));
            fds_assert(happened);
        }
        return blob_it->second;
    }
    return nullptr;
}

size_t
AmBlockLayer::split_update(PutBlobReq* blobReq, std::deque<sector_type>& needed) {
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
        sector_type update {blobReq, aligned_off, part_off, objBuf, complete};
        if (offset_queue->queue_update(update)) {
            // This is a RMW and we don't have the data already, and
            // we need to dispatch a get to DM/SM for this data
            needed.emplace_back(update);
        }
    }
    return objects;
}

void
AmBlockLayer::dispatchReads(AmRequest* parent_request,
                            std::deque<sector_type>& need_dispatch) {
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
        getReq->for_rmw = true;
        AmDataProvider::getBlob(getReq);
    }
}


}  // namespace fds
