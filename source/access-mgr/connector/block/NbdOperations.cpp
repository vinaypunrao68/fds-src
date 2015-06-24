/**
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <algorithm>
#include <map>
#include <string>
#include <utility>

#include "connector/block/common.h"
#include "connector/block/NbdOperations.h"

#include "AmAsyncDataApi_impl.h"

namespace fds {

/**
 * Since multiple connections can serve the same volume we need
 * to keep this association information somewhere so we can
 * properly detach from the volume when unused.
 */
static std::unordered_map<std::string, std::uint_fast16_t> assoc_map {};
static std::mutex assoc_map_lock {};


void
NbdResponseVector::handleReadResponse(std::vector<boost::shared_ptr<std::string>>& buffers,
                                      fds_uint32_t len) {
    static boost::shared_ptr<std::string> const empty_buffer =
        boost::make_shared<std::string>(maxObjectSizeInBytes, '\0');
    fds_assert(operation == READ);

    // acquire the buffers
    bufVec.swap(buffers);

    // Fill in any missing wholes with zero data, this is a special *block*
    // semantic for NULL objects.
    for (auto& buf: bufVec) {
        if (!buf) {
            buf = empty_buffer;
            len += maxObjectSizeInBytes;
        }
    }

    // return zeros for uninitialized objects, again a special *block*
    // semantic to PAD the read to the required length.
    if (length > len) {
        for (int64_t zero_data = length - len; 0 < zero_data; zero_data -= maxObjectSizeInBytes) {
            bufVec.push_back(empty_buffer);
        }
    }

    // Trim the data as needed from the front...
    fds_uint32_t iOff = offset % maxObjectSizeInBytes;
    auto firstObjLen = std::min(length, maxObjectSizeInBytes - iOff);
    if (maxObjectSizeInBytes != firstObjLen) {
        bufVec.front() = boost::make_shared<std::string>(bufVec.front()->data() + iOff, firstObjLen);
    }

    // ...and the back
    if (length > firstObjLen) {
        auto lastObjLen = length - firstObjLen - (bufVec.size()-2) * maxObjectSizeInBytes;
        if (0 < lastObjLen && maxObjectSizeInBytes != lastObjLen) {
            bufVec.back() = boost::make_shared<std::string>(bufVec.back()->data(), lastObjLen);
        }
    }
}

std::pair<fds_uint64_t, boost::shared_ptr<std::string>>
NbdResponseVector::handleRMWResponse(boost::shared_ptr<std::string> retBuf,
                                 fds_uint32_t len,
                                 fds_uint32_t seqId,
                                 const Error& err) {
    fds_assert(operation == WRITE);
    if (!err.ok() && (err != ERR_BLOB_OFFSET_INVALID) &&
                     (err != ERR_BLOB_NOT_FOUND)) {
        opError = err;
        LOGERROR << "Error: " << err << " for: 0x" << std::hex << handle;
        return std::make_pair(offVec[seqId], boost::shared_ptr<std::string>());
    }

    fds_uint32_t iOff = (seqId == 0) ? offset % maxObjectSizeInBytes : 0;
    boost::shared_ptr<std::string>& writeBytes = bufVec[seqId];
    boost::shared_ptr<std::string> fauxBytes;
    if ((err == ERR_BLOB_OFFSET_INVALID) || (err == ERR_BLOB_NOT_FOUND) ||
        0 == len || !retBuf) {
        // we tried to read unwritten block, so create
        // an empty block buffer to place the data
        fauxBytes = boost::make_shared<std::string>(maxObjectSizeInBytes, '\0');
        fauxBytes->replace(iOff, writeBytes->length(),
                           writeBytes->c_str(), writeBytes->length());
    } else {
        fds_assert(len == maxObjectSizeInBytes);
        // Need to copy retBut into a modifiable buffer since retBuf is owned
        // by AM and should not be modified here.
        // TODO(Andrew): Make retBuf a const
        fauxBytes = boost::make_shared<std::string>(retBuf->c_str(), retBuf->length());
        fauxBytes->replace(iOff, writeBytes->length(),
                           writeBytes->c_str(), writeBytes->length());
    }
    return std::make_pair(offVec[seqId], fauxBytes);
}

NbdOperations::NbdOperations(NbdOperationsResponseIface* respIface)
        : amAsyncDataApi(nullptr),
          volumeName(nullptr),
          nbdResp(respIface),
          domainName(new std::string("TestDomain")),
          blobName(new std::string("BlockBlob")),
          emptyMeta(new std::map<std::string, std::string>()),
          blobMode(new fds_int32_t(0)),
          sector_map()
{
}

// We can't initialize this in the constructor since we want to pass
// a shared pointer to ourselves (and NbdConnection already started one).
void
NbdOperations::init(boost::shared_ptr<std::string> vol_name,
                    std::shared_ptr<AmProcessor> processor)
{
    amAsyncDataApi.reset(new AmAsyncDataApi<handle_type>(processor, shared_from_this()));
    volumeName = vol_name;

    handle_type reqId{0, 0};

    // We assume the client wants R/W with a cache for now
    auto mode = boost::make_shared<fpi::VolumeAccessMode>();
    amAsyncDataApi->attachVolume(reqId,
                                 domainName,
                                 volumeName,
                                 mode);
}

void
NbdOperations::attachVolumeResp(const Error& error,
                                handle_type& requestId,
                                boost::shared_ptr<VolumeDesc>& volDesc,
                                boost::shared_ptr<fpi::VolumeAccessMode>& mode) {
    LOGDEBUG << "Reponse for attach: [" << error << "]";
    if (ERR_OK == error) {
        if (fpi::FDSP_VOL_BLKDEV_TYPE != volDesc->volType) {
            LOGWARN << "Wrong volume type: " << volDesc->volType;
            return nbdResp->attachResp(ERR_INVALID_VOL_ID, nullptr);
        }
        maxObjectSizeInBytes = volDesc->maxObjSizeInBytes;

        // Reference count this association
        std::unique_lock<std::mutex> lk(assoc_map_lock);
        ++assoc_map[volDesc->name];
    }
    nbdResp->attachResp(error, volDesc);
}

void
NbdOperations::detachVolume() {
    if (volumeName) {
        // Only close the volume if it's the last connection
        std::unique_lock<std::mutex> lk(assoc_map_lock);
        if (0 == --assoc_map[*volumeName]) {
            handle_type reqId{0, 0};
            amAsyncDataApi->detachVolume(reqId, domainName, volumeName);
        }
    }
}

void
NbdOperations::detachVolumeResp(const Error& error,
                                handle_type& requestId) {
    // Volume detach has completed, we shaln't use the volume again
    LOGDEBUG << "Volume detach response: " << error;
    amAsyncDataApi.reset();
}

void
NbdOperations::read(fds_uint32_t length,
                    fds_uint64_t offset,
                    fds_int64_t handle) {
    fds_assert(amAsyncDataApi);

    LOGDEBUG << "Want 0x" << std::hex << length << " bytes from 0x" << offset
             << " handle 0x" << handle << std::dec;

    // we will wait for responses
    NbdResponseVector* resp = new NbdResponseVector(handle,
                                                    NbdResponseVector::READ,
                                                    offset, length, maxObjectSizeInBytes);


    {   // add response that we will fill in with data
        fds_mutex::scoped_lock l(respLock);
        if (false == responses.emplace(std::make_pair(handle, resp)).second)
            { throw NbdError::connection_closed; }
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

    handle_type reqId{handle, 0};
    amAsyncDataApi->getBlob(reqId,
                            domainName,
                            volumeName,
                            blobName,
                            blobLength,
                            off);
}


void
NbdOperations::write(boost::shared_ptr<std::string>& bytes,
                     fds_uint32_t length,
                     fds_uint64_t offset,
                     fds_int64_t handle) {
    fds_assert(amAsyncDataApi);
    // calculate how many FDS objects we will write
    fds_uint32_t objCount = getObjectCount(length, offset);

    // we will wait for write response for all objects we chunk this request into
    NbdResponseVector* resp = new NbdResponseVector(handle,
                                                    NbdResponseVector::WRITE,
                                                    offset, length,
                                                    maxObjectSizeInBytes, objCount);

    {   // add response that we will fill in with data
        fds_mutex::scoped_lock l(respLock);
        if (false == responses.emplace(std::make_pair(handle, resp)).second)
            { throw NbdError::connection_closed; }
    }

    size_t amBytesWritten = 0;
    uint32_t seqId = 0;
    while (amBytesWritten < length) {
        fds_uint64_t curOffset = offset + amBytesWritten;
        fds_uint64_t objectOff = curOffset / maxObjectSizeInBytes;
        fds_uint32_t iOff = curOffset % maxObjectSizeInBytes;
        size_t iLength = length - amBytesWritten;

        if ((iLength + iOff) >= maxObjectSizeInBytes) {
            iLength = maxObjectSizeInBytes - iOff;
        }

        LOGTRACE  << "Will write offset: 0x" << std::hex << curOffset
                  << " for length: 0x" << iLength;

        auto objBuf = (iLength == bytes->length()) ?
            bytes : boost::make_shared<std::string>(*bytes, amBytesWritten, iLength);

        // write an object
        boost::shared_ptr<int32_t> objLength = boost::make_shared<int32_t>(maxObjectSizeInBytes);
        boost::shared_ptr<apis::ObjectOffset> off(new apis::ObjectOffset());
        off->value = objectOff;

        // request id is 64 bit of handle + 32 bit of sequence Id
        handle_type reqId{handle, seqId};

        // To prevent a race condition we lock the sector (object) for the
        // duration of the operation and queue other requests to that offset
        // behind it. When the operation finishes it pull the next op off the
        // queue and enqueue it to QoS
        resp->keepBufferForWrite(seqId, objectOff, objBuf);
        if (sector_type::QueueResult::FirstEntry ==
                sector_map.queue_update(objectOff, reqId)) {
            if (iLength != maxObjectSizeInBytes) {
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
NbdOperations::getBlobResp(const Error &error,
                           handle_type& requestId,
                           const boost::shared_ptr<std::vector<boost::shared_ptr<std::string>>>& bufs,
                           int& length) {
    NbdResponseVector* resp = NULL;
    fds_int64_t handle = requestId.handle;
    uint32_t seqId = requestId.seq;

    LOGDEBUG << "Reponse for getBlob, " << length << " bytes "
             << error << ", handle " << handle
             << " seqId " << seqId;

    {
        fds_mutex::scoped_lock l(respLock);
        // if we are not waiting for this response, we probably already
        // returned an error
        auto it = responses.find(handle);
        if (responses.end() == it) {
            LOGWARN << "Not waiting for response for handle 0x" << std::hex << handle << std::dec
                    << ", check if we returned an error";
            return;
        }
        // get response
        resp = it->second;
    }

    fds_verify(resp);
    if (!resp->isRead()) {
        static NbdResponseVector::buffer_ptr_type const null_buff(nullptr);
        // this is a response for read during a write operation from NBD connector
        LOGDEBUG << "Write after read, handle 0x" << std::hex << handle << std::dec << " seqId " << seqId;

        // RMW only operates on a single buffer...
        auto& buf = (bufs && !bufs->empty()) ? bufs->front() : null_buff;

        // apply the update from NBD connector to this object
        auto rwm_pair = resp->handleRMWResponse(buf, length, seqId, error);
        if (rwm_pair.second) {
           // rwm_pair.second contains object updated with data from NBD connector
           // rwm_pair.first is the object offset for this buffer
            boost::shared_ptr<int32_t> objLength =
                boost::make_shared<int32_t>(rwm_pair.second->length());
            boost::shared_ptr<apis::ObjectOffset> off(new apis::ObjectOffset());
            off->value = rwm_pair.first;
            amAsyncDataApi->updateBlobOnce(requestId,
                                           domainName,
                                           volumeName,
                                           blobName,
                                           blobMode,
                                           rwm_pair.second,
                                           objLength,
                                           off,
                                           emptyMeta);
            return;
        }
    } else {
        // this is response for read operation,
        if (error.ok() || (error == ERR_BLOB_OFFSET_INVALID) ||
                          (error == ERR_BLOB_NOT_FOUND)) {
            // Adjust the buffers in our vector so they align and are of the
            // correct length according to the original request
            resp->handleReadResponse(*bufs, length);
        }
    }

    bool response_removed = finishResponse(handle);
    if (response_removed) {
        // we are done collecting responses for this handle, notify nbd connector
        nbdResp->readWriteResp(resp);
    } else {
        LOGNOTIFY << "Handle 0x" << std::hex << handle << std::dec
                  << " was missing from the response map!";
    }
}

void
NbdOperations::updateBlobResp(const Error &error,
                              handle_type& requestId) {
    NbdResponseVector* resp = nullptr;
    fds_int64_t handle = requestId.handle;
    uint32_t seqId = requestId.seq;

    LOGDEBUG << "Reponse for updateBlobOnce, "
             << error << ", handle " << handle
             << " seqId " << seqId;

    {
        fds_mutex::scoped_lock l(respLock);
        // if we are not waiting for this response, we probably already
        // returned an error
        auto it = responses.find(handle);
        if (responses.end() == it) {
            LOGWARN << "Not waiting for response for handle " << handle
                    << ", check if we returned an error";
            return;
        }
        // get response
        resp = it->second;
    }

    // Unblock other updates on the same object if they exist
    auto offset = resp->getOffset(seqId);
    bool update_queued;
    handle_type queued_handle;
    std::tie(update_queued, queued_handle) = sector_map.pop_and_delete(offset);
    if (update_queued) {
        NbdResponseVector* queued_resp = nullptr;
        {
            fds_mutex::scoped_lock l(respLock);
            auto it = responses.find(queued_handle.handle);
            if (responses.end() != it) {
                queued_resp = it->second;
            }
        }
        if (queued_resp) {
            auto buffer = queued_resp->getBuffer(queued_handle.seq);
            boost::shared_ptr<apis::ObjectOffset> off(new apis::ObjectOffset());
            off->value = offset;
            boost::shared_ptr<int32_t> objLength =
                boost::make_shared<int32_t>(maxObjectSizeInBytes);
            if (maxObjectSizeInBytes != buffer->length()) {
                amAsyncDataApi->getBlob(queued_handle,
                                        domainName,
                                        volumeName,
                                        blobName,
                                        objLength,
                                        off);
            } else {
                amAsyncDataApi->updateBlobOnce(queued_handle,
                                               domainName,
                                               volumeName,
                                               blobName,
                                               blobMode,
                                               buffer,
                                               objLength,
                                               off,
                                               emptyMeta);
            }
        } else {
            fds_panic("Missing response vector for update!");
        }
    }

    // update response vector
    fds_verify(resp);
    fds_bool_t done = resp->handleWriteResponse(seqId, error);
    if (done) {
        if (finishResponse(handle)) {
            // we are done collecting responses for this handle, notify nbd connector
            nbdResp->readWriteResp(resp);
        } else {
            LOGNOTIFY << "Handle 0x" << std::hex << handle << std::dec
                      << " was missing from the response map!";
        }
    }
}

bool
NbdOperations::finishResponse(fds_int64_t const handle) {
    // nbd connector will free resp, just accounting here
    bool done_responding, response_removed;
    {
        fds_mutex::scoped_lock l(respLock);
        response_removed = (1 == responses.erase(handle));
        done_responding = responses.empty();
    }
    // Only one response will ever see shutting_down == true and
    // no responses left, safe to do this now.
    if (shutting_down && done_responding) {
        LOGDEBUG << "Nbd responses drained, finalizing detach from: " << *volumeName;
        detachVolume();
    }
    return response_removed;
}

void
NbdOperations::shutdown()
{
    fds_mutex::scoped_lock l(respLock);
    shutting_down = true;
    // If we don't have any outstanding requests, we're done
    if (responses.empty()) {
        amAsyncDataApi.reset();
    }
}

/**
 * Calculate number of objects that are contained in a request with length
 * 'length' at offset 'offset'
 */
fds_uint32_t
NbdOperations::getObjectCount(fds_uint32_t length,
                              fds_uint64_t offset) {
    fds_uint32_t objCount = length / maxObjectSizeInBytes;
    fds_uint32_t firstObjLen = maxObjectSizeInBytes - (offset % maxObjectSizeInBytes);
    if ((length % maxObjectSizeInBytes) != 0) {
        ++objCount;
    }
    if ((firstObjLen + (objCount - 1) * maxObjectSizeInBytes) < length) {
        ++objCount;
    }
    return objCount;
}

}  // namespace fds
