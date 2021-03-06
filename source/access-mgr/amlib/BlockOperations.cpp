/**
 * Copyright 2015 by Formation Data Systems, Inc.
 */

#include "connector/BlockOperations.h"

#include <algorithm>
#include <deque>
#include <map>
#include <string>
#include <thread>
#include <utility>

#include "fds_volume.h"
#include "util/Log.h"

namespace fds {

/**
 * Since multiple connections can serve the same volume we need
 * to keep this association information somewhere so we can
 * properly detach from the volume when unused.
 */
static std::unordered_map<std::string, std::uint_fast16_t> assoc_map {};
static std::mutex assoc_map_lock {};

BlockOperations::BlockOperations(BlockOperations::ResponseIFace* respIface)
        : amAsyncDataApi(nullptr),
          volumeName(nullptr),
          blockResp(respIface),
          domainName(new std::string("TestDomain")),
          blobName(new std::string("BlockBlob")),
          emptyMeta(new std::map<std::string, std::string>()),
          blobMode(new int32_t(0)),
          sector_map()
{
}

// We can't initialize this in the constructor since we want to pass
// a shared pointer to ourselves (and the Connection already started one).
void
BlockOperations::init(boost::shared_ptr<std::string> vol_name,
                      std::shared_ptr<AmProcessor> processor,
                      BlockTask* resp,
                      uint32_t const obj_size)
{
    if (!amAsyncDataApi) {
        amAsyncDataApi.reset(new AmAsyncDataApi(processor, shared_from_this()));
        volumeName = vol_name;
    }

    if (resp) {   // add response that we will fill in with data
        std::unique_lock<std::mutex> l(respLock);
        if (false == responses.emplace(std::make_pair(resp->getHandle(), resp)).second)
            { throw BlockError::connection_closed; }
    }

    // If we don't know the object size yet (NBD) we need to look it up
    if (0 == obj_size) {
        // We assume the client wants R/W with a cache for now
        handle_type reqId{resp->getHandle(), 0};
        auto mode = boost::make_shared<fpi::VolumeAccessMode>();
        mode->can_write = false;
        mode->can_cache = false;
        amAsyncDataApi->attachVolume(reqId, domainName, volumeName, mode);
    } else {
        fds_assert(!resp); // Shouldn't have a task
        maxObjectSizeInBytes = obj_size;
        empty_buffer = boost::make_shared<std::string>(maxObjectSizeInBytes, '\0');

        // Reference count this association
        std::unique_lock<std::mutex> lk(assoc_map_lock);
        ++assoc_map[*volumeName];
    }
}

void
BlockOperations::attachVolumeResp(const fpi::ErrorCode& error,
                                handle_type const& requestId,
                                boost::shared_ptr<VolumeDesc>& volDesc,
                                boost::shared_ptr<fpi::VolumeAccessMode>& mode) {
    LOGDEBUG << "err:" << error << " attach response";
    auto handle = requestId.handle;
    BlockTask* resp = NULL;

    {
        std::unique_lock<std::mutex> l(respLock);
        // if we are not waiting for this response, we probably already
        // returned an error
        auto it = responses.find(handle);
        if (responses.end() == it) {
            LOGWARN << "handle:" << handle << " not awaiting response, check for error";
            return;
        }
        // get response
        resp = it->second;
    }

    boost::shared_ptr<VolumeDesc> descriptor = nullptr;
    if (fpi::OK == error) {
        if (fpi::FDSP_VOL_BLKDEV_TYPE != volDesc->volType &&
            fpi::FDSP_VOL_ISCSI_TYPE != volDesc->volType) {
            LOGWARN << "wrong volume type:" << volDesc->volType;
            resp->setError(fpi::BAD_REQUEST);
        } else {
            maxObjectSizeInBytes = volDesc->maxObjSizeInBytes;
            empty_buffer = boost::make_shared<std::string>(maxObjectSizeInBytes, '\0');

            // Reference count this association
            std::unique_lock<std::mutex> lk(assoc_map_lock);
            ++assoc_map[volDesc->name];
            descriptor = volDesc;
        }
    } else {
        resp->setError(error);
    }
    blockResp->attachResp(descriptor);
    finishResponse(resp);
}

void
BlockOperations::detachVolume() {
    if (volumeName) {
        // Only close the volume if it's the last connection
        std::unique_lock<std::mutex> lk(assoc_map_lock);
        auto itr = assoc_map.find(*volumeName);
        std::unique_lock<std::mutex> ul(shutdownLock);
        if ((assoc_map.end() != itr) && ((true == shutting_down) || (0 == --assoc_map[*volumeName]))) {
            ul.unlock();
            handle_type reqId{0, 0};
            assoc_map.erase(*volumeName);
            return amAsyncDataApi->detachVolume(reqId, domainName, volumeName);
        }
        ul.unlock();
    }
    // If we weren't attached, pretend if we had been to be DRY
    handle_type fake_req;
    detachVolumeResp(fpi::OK, fake_req);
}

void
BlockOperations::detachVolumeResp(const fpi::ErrorCode& error,
                                handle_type const& requestId) {
    // Volume detach has completed, we shaln't use the volume again
    std::lock_guard<std::mutex> lg(shutdownLock);
    LOGDEBUG << "err:" << error << " detach response";
    if ((true == shutting_down) && (nullptr != blockResp)) {
        blockResp->terminate();
        blockResp = nullptr;
        amAsyncDataApi.reset();
    }
}

void
BlockOperations::read(BlockTask* resp) {
    fds_assert(amAsyncDataApi);

    resp->setMaxObjectSize(maxObjectSizeInBytes);
    auto length = resp->getLength();
    auto offset = resp->getOffset();

    {   // add response that we will fill in with data
        std::unique_lock<std::mutex> l(respLock);
        if (false == responses.emplace(std::make_pair(resp->getHandle(), resp)).second)
            { throw BlockError::connection_closed; }
    }

    // Determine how much data we need to read, we need
    // to read the entire object and the offset needs to be aligned
    length = length + (offset % maxObjectSizeInBytes);
    auto overrun = length % maxObjectSizeInBytes;
    length += (0 < overrun) ? maxObjectSizeInBytes - overrun : 0;

    // we will read region of length from AM
    boost::shared_ptr<int32_t> blobLength = boost::make_shared<int32_t>(length);
    boost::shared_ptr<apis::ObjectOffset> off(new apis::ObjectOffset());
    off->value = offset / maxObjectSizeInBytes;

    handle_type reqId{resp->getHandle(), 0};
    amAsyncDataApi->getBlob(reqId,
                            domainName,
                            volumeName,
                            blobName,
                            blobLength,
                            off);
}

void
BlockOperations::write(typename req_api_type::shared_buffer_type& bytes, task_type* resp) {
    fds_assert(amAsyncDataApi);
    // calculate how many FDS objects we will write
    auto length = resp->getLength();
    auto offset = resp->getOffset();
    uint32_t objCount = getObjectCount(length, offset);

    resp->setMaxObjectSize(maxObjectSizeInBytes);
    resp->setObjectCount(objCount);

    {   // add response that we will fill in with data
        std::unique_lock<std::mutex> l(respLock);
        if (false == responses.emplace(std::make_pair(resp->getHandle(), resp)).second)
            { throw BlockError::connection_closed; }
    }

    size_t amBytesWritten = 0;
    uint32_t seqId = 0;
    while (amBytesWritten < length) {
        uint64_t curOffset = offset + amBytesWritten;
        uint64_t objectOff = curOffset / maxObjectSizeInBytes;
        uint32_t iOff = curOffset % maxObjectSizeInBytes;
        size_t iLength = length - amBytesWritten;

        if ((iLength + iOff) >= maxObjectSizeInBytes) {
            iLength = maxObjectSizeInBytes - iOff;
        }

        LOGTRACE  << "offset: " << curOffset << " length:" << iLength << " write request";

        auto objBuf = (iLength == bytes->length()) ?
            bytes : boost::make_shared<std::string>(*bytes, amBytesWritten, iLength);

        // write an object
        boost::shared_ptr<int32_t> objLength = boost::make_shared<int32_t>(maxObjectSizeInBytes);
        boost::shared_ptr<apis::ObjectOffset> off(new apis::ObjectOffset());
        off->value = objectOff;

        // request id is 64 bit of handle + 32 bit of sequence Id
        handle_type reqId{resp->getHandle(), seqId};

        // To prevent a race condition we lock the sector (object) for the
        // duration of the operation and queue other requests to that offset
        // behind it. When the operation finishes it pull the next op off the
        // queue and enqueue it to QoS
        auto partial_write = (iLength != maxObjectSizeInBytes);
        resp->keepBufferForWrite(seqId, objectOff, objBuf);
        if (sector_type::QueueResult::FirstEntry ==
                sector_map.queue_update(objectOff, reqId)) {
            if (partial_write) {
                // For objects that we are only updating a part of, we need to
                // perform a Read-Modify-Write operation, keep the data for the
                // update to the first and last object in the response so that
                // we can apply the update to the object on read response
                amAsyncDataApi->getBlob(reqId,
                                        domainName,
                                        volumeName,
                                        blobName,
                                        objLength,
                                        off);
            } else {
                amAsyncDataApi->updateBlobOnce(reqId,
                                               domainName,
                                               volumeName,
                                               blobName,
                                               blobMode,
                                               objBuf,
                                               objLength,
                                               off,
                                               emptyMeta);
            }
        }
        amBytesWritten += iLength;
        ++seqId;
    }
}

void
BlockOperations::getBlobResp(const fpi::ErrorCode &error,
                           handle_type const& requestId,
                           const boost::shared_ptr<std::vector<boost::shared_ptr<std::string>>>& bufs,
                           int& length) {
    BlockTask* resp = nullptr;
    auto handle = requestId.handle;
    uint32_t seqId = requestId.seq;

    LOGDEBUG << "handle:" << handle << " seqid:" << seqId
             << " err:" << error << " length:" << length << " getBlob response";

    {
        std::unique_lock<std::mutex> l(respLock);
        // if we are not waiting for this response, we probably already
        // returned an error
        auto it = responses.find(handle);
        if (responses.end() == it) {
            LOGWARN << "handle:" << handle << " not awaiting response, check for error";
            return;
        }
        // get response
        resp = it->second;
    }

    fds_verify(resp);
    if (!resp->isRead()) {
        static BlockTask::buffer_ptr_type const null_buff(nullptr);
        // this is a response for read during a write operation from Block connector
        LOGDEBUG << "handle:" << handle << " seqid:" << seqId << " write after read";

        // RMW only operates on a single buffer...
        auto buf = (bufs && !bufs->empty()) ? bufs->front() : null_buff;

        // Chain all pending updates together and issue update
        return drainUpdateChain(resp->getOffset(seqId), buf, &requestId, error);
    }

    // this is response for read operation,
    if (fpi::OK == error || fpi::MISSING_RESOURCE == error) {
        // Adjust the buffers in our vector so they align and are of the
        // correct length according to the original request
        resp->handleReadResponse(*bufs, empty_buffer, length);
    } else {
        resp->setError(error);
    }
    finishResponse(resp);
}

void
BlockOperations::updateBlobOnceResp(const fpi::ErrorCode &error, handle_type const& requestId) {
    BlockTask* resp = nullptr;
    auto const& handle = requestId.handle;
    auto const& seqId = requestId.seq;
    uint64_t offset {0};

    LOGDEBUG << "handle:" << handle << " seqid:" << seqId
             << " err:" << error << " updateBlobOnce response";

    {
        std::unique_lock<std::mutex> l(respLock);
        // if we are not waiting for this response, we probably already
        // returned an error
        auto it = responses.find(handle);
        if (responses.end() == it) {
            LOGWARN << "handle:" << handle << " not awaiting response, check for error";
            return;
        }
        // get response
        resp = it->second;
        fds_assert(resp);

        offset = resp->getOffset(seqId);
    }

    // Unblock other updates on the same object if they exist
    drainUpdateChain(offset, resp->getBuffer(seqId), nullptr, error);

    // respond to all chained requests FIRST
    std::deque<BlockTask*> chained_responses;
    resp->getChain(seqId, chained_responses);
    for (auto chained_resp : chained_responses) {
        if (chained_resp->handleWriteResponse(error)) {
            finishResponse(chained_resp);
        }
    }

    if (resp->handleWriteResponse(error)) {
        finishResponse(resp);
    }
}

void
BlockOperations::drainUpdateChain(uint64_t const offset,
                                BlockTask::buffer_ptr_type buf,
                                handle_type const* queued_handle_ptr,
                                fpi::ErrorCode const error) {
    // The first call to handleRMWResponse will create a null buffer if this is
    // an error, afterwards fpi::OK for everyone.
    auto err = error;
    bool update_queued {true};
    handle_type queued_handle;
    BlockTask* last_chained = nullptr;
    std::deque<BlockTask*> chain;

    //  Either we explicitly are handling a RMW or checking to see if there are
    //  any new ones in the queue
    if (nullptr == queued_handle_ptr) {
        std::tie(update_queued, queued_handle) = sector_map.pop(offset, true);
    } else {
        queued_handle = *queued_handle_ptr;
    }

    while (update_queued) {
        BlockTask* queued_resp = nullptr;
        {
            std::unique_lock<std::mutex> l(respLock);
            auto it = responses.find(queued_handle.handle);
            if (responses.end() != it) {
                queued_resp = it->second;
            }
        }
        if (queued_resp) {
            auto new_data = queued_resp->getBuffer(queued_handle.seq);
            if (maxObjectSizeInBytes != new_data->length()) {
                std::tie(err, new_data) = queued_resp->handleRMWResponse(buf,
                                                                         maxObjectSizeInBytes,
                                                                         queued_handle.seq,
                                                                         err);
            }

            // Respond to request if error
            if (fpi::OK != err) {
                if (queued_resp->handleWriteResponse(err)) {
                    finishResponse(queued_resp);
                }
            } else {
                buf = new_data;
                if (nullptr != last_chained) {
                    // If we're chaining, use the last chain, copy the chain
                    // and add it to the chain
                    chain.push_back(last_chained);
                }
                last_chained = queued_resp;
            }
        } else {
            fds_panic("Missing response vector for update!");
        }
        handle_type next_handle;
        std::tie(update_queued, next_handle) = sector_map.pop(offset, nullptr == last_chained);
        // Leave queued_handle pointing to the last handle
        if (update_queued) {
            queued_handle = next_handle;
        }
    }

    // Update the blob if we have updates to make
    if (nullptr != last_chained) {
        last_chained->setChain(queued_handle.seq, std::move(chain));
        auto objLength = boost::make_shared<int32_t>(maxObjectSizeInBytes);
        auto off = boost::make_shared<apis::ObjectOffset>();
        off->value = offset;
        amAsyncDataApi->updateBlobOnce(queued_handle,
                                       domainName,
                                       volumeName,
                                       blobName,
                                       blobMode,
                                       buf,
                                       objLength,
                                       off,
                                       emptyMeta);
    }
}


void
BlockOperations::finishResponse(BlockTask* response) {
    // block connector will free resp, just accounting here
    bool done_responding, response_removed;
    {
        std::unique_lock<std::mutex> l(respLock);
        response_removed = (1 == responses.erase(response->getHandle()));
        done_responding = responses.empty();
    }
    if (response_removed) {
        blockResp->respondTask(response);
    } else {
        LOGNOTIFY << "handle:" << response->getHandle() << " missing from response map";
    }

    // Only one response will ever see shutting_down == true and
    // no responses left, safe to do this now.
    std::unique_lock<std::mutex> ul(shutdownLock);
    if (shutting_down && done_responding) {
        ul.unlock();
        LOGDEBUG << "vol:" << *volumeName << " block responses drained, detaching";
        detachVolume();
    }
}

void
BlockOperations::shutdown()
{
    std::unique_lock<std::mutex> ul(shutdownLock);
    if (shutting_down) return;
    shutting_down = true;
    ul.unlock();
    std::unique_lock<std::mutex> l(respLock);
    // If we don't have any outstanding requests, we're done
    if (responses.empty()) {
        detachVolume();
    }
}

/**
 * Calculate number of objects that are contained in a request with length
 * 'length' at offset 'offset'
 */
uint32_t
BlockOperations::getObjectCount(uint32_t length,
                              uint64_t offset) {
    uint32_t objCount = length / maxObjectSizeInBytes;
    uint32_t firstObjLen = maxObjectSizeInBytes - (offset % maxObjectSizeInBytes);
    if ((length % maxObjectSizeInBytes) != 0) {
        ++objCount;
    }
    if ((firstObjLen + (objCount - 1) * maxObjectSizeInBytes) < length) {
        ++objCount;
    }
    return objCount;
}

}  // namespace fds
