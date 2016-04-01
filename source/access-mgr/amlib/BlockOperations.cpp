/**
 * Copyright 2015-2016 by Formation Data Systems, Inc.
 */

#include "connector/BlockOperations.h"

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
          blobMode(new int32_t(0))
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
        if ((assoc_map.end() != itr) && (0 == --assoc_map[*volumeName])) {
            handle_type reqId{0, 0};
            assoc_map.erase(*volumeName);
            return amAsyncDataApi->detachVolume(reqId, domainName, volumeName);
        }
    }
    // If we weren't attached, pretend if we had been to be DRY
    handle_type fake_req;
    detachVolumeResp(fpi::OK, fake_req);
}

// Will aquire respLock and then call _detachVolumeResp
void
BlockOperations::detachVolumeResp(const fpi::ErrorCode& error,
                                  handle_type const& requestId) {
    std::unique_lock<std::mutex> l(respLock);
    _detachVolumeResp(error, requestId);
}

// Use if already holding respLock
void
BlockOperations::_detachVolumeResp(const fpi::ErrorCode& error,
                                handle_type const& requestId) {
    // Volume detach has completed, we shaln't use the volume again
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
    off->value = (offset / maxObjectSizeInBytes) * maxObjectSizeInBytes;

    handle_type reqId{resp->getHandle(), 0};
    amAsyncDataApi->getBlob(reqId,
                            domainName,
                            volumeName,
                            blobName,
                            blobLength,
                            off,
                            true);
}

void
BlockOperations::write(typename req_api_type::shared_buffer_type& bytes, task_type* resp) {
    fds_assert(amAsyncDataApi);
    // calculate how many FDS objects we will write
    auto length = resp->getLength();
    auto offset = resp->getOffset();

    {   // add response that we will fill in with data
        std::unique_lock<std::mutex> l(respLock);
        if (false == responses.emplace(std::make_pair(resp->getHandle(), resp)).second)
            { throw BlockError::connection_closed; }
    }

    handle_type reqId{resp->getHandle(), 0};
    boost::shared_ptr<int32_t> objLength = boost::make_shared<int32_t>(length);
    boost::shared_ptr<apis::ObjectOffset> off(new apis::ObjectOffset());
    off->value = offset;
    amAsyncDataApi->updateBlobOnce(reqId, domainName, volumeName, blobName, blobMode,
                                   bytes, objLength, off, emptyMeta, true);
}

void
BlockOperations::getBlobResp(const fpi::ErrorCode &error,
                           handle_type const& requestId,
                           const boost::shared_ptr<std::vector<boost::shared_ptr<std::string>>>& bufs,
                           int& length) {
    BlockTask* resp = nullptr;
    auto handle = requestId.handle;

    LOGDEBUG << "handle:" << handle
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

    if (fpi::OK == error || fpi::MISSING_RESOURCE == error) {
        // Adjust the buffers in our vector so they align and are of the
        // correct length according to the original request
        resp->handleReadResponse(*bufs, empty_buffer, length, maxObjectSizeInBytes);
    } else {
        resp->setError(error);
    }
    finishResponse(resp);
}

void
BlockOperations::updateBlobOnceResp(const fpi::ErrorCode &error, handle_type const& requestId) {
    BlockTask* resp = nullptr;
    auto const& handle = requestId.handle;

    LOGDEBUG << "handle:" << handle
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
    }

    resp->setError(error);
    finishResponse(resp);
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
        LOGNOTIFY << "handle:" << response->getHandle() << std::dec << " missing from response map";
    }

    // Only one response will ever see shutting_down == true and
    // no responses left, safe to do this now.
    if (shutting_down && done_responding) {
        LOGDEBUG << "vol:" << *volumeName << " block responses drained, detaching";
        detachVolume();
    }
}

void
BlockOperations::shutdown()
{
    std::unique_lock<std::mutex> l(respLock);
    if (shutting_down) return;
    shutting_down = true;
    // If we don't have any outstanding requests, we're done
    if (responses.empty()) {
        {
            std::unique_lock<std::mutex> lk(assoc_map_lock);
            assoc_map.erase(*volumeName);
        }
        handle_type fake_req;
        _detachVolumeResp(fpi::OK, fake_req);
    }
}

}  // namespace fds
