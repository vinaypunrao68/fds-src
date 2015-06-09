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


fds_bool_t
NbdResponseVector::handleReadResponse(std::vector<boost::shared_ptr<std::string>>& buffers,
                                      fds_uint32_t len,
                                      const Error& err) {
    fds_assert(operation == READ);

    if (!err.ok() && (err != ERR_BLOB_OFFSET_INVALID) &&
                     (err != ERR_BLOB_NOT_FOUND)) {
        return true;
    }

    // acquire the buffers
    bufVec.swap(buffers);

    // return zeros for uninitialized objects
    if (length > len) {
        for (auto zero_data = length - len; 0 < zero_data;) {
            auto buf_size = std::min(zero_data, maxObjectSizeInBytes);
            bufVec.emplace_back(new std::string(buf_size, '\0'));
            zero_data -= buf_size;
        }
    }
    // Fill in any missing wholes with zero data
    for (auto& buf: bufVec) {
        if (!buf) {
            buf = boost::make_shared<std::string>(maxObjectSizeInBytes, '\0');
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
        if (0 < lastObjLen) {
            bufVec.back() = boost::make_shared<std::string>(bufVec.back()->data(), lastObjLen);
        }
    }
    return true;
}

std::pair<fds_uint64_t, boost::shared_ptr<std::string>>
NbdResponseVector::handleRMWResponse(boost::shared_ptr<std::string> retBuf,
                                 fds_uint32_t len,
                                 fds_uint32_t seqId,
                                 const Error& err) {
    fds_assert(operation == WRITE);
    fds_uint32_t index = (seqId == 0) ? 0 : 1;
    if (!err.ok() && (err != ERR_BLOB_OFFSET_INVALID) &&
                     (err != ERR_BLOB_NOT_FOUND)) {
        opError = err;
        LOGERROR << "Error: " << err << " for: 0x" << std::hex << handle;
        return std::make_pair(offVec[index], boost::shared_ptr<std::string>());
    }

    fds_uint32_t iOff = (seqId == 0) ? offset % maxObjectSizeInBytes : 0;
    boost::shared_ptr<std::string>& writeBytes = bufVec[index];
    boost::shared_ptr<std::string> fauxBytes;
    if ((err == ERR_BLOB_OFFSET_INVALID) || (err == ERR_BLOB_NOT_FOUND) ||
        0 == len || retBuf->empty()) {
        // we tried to read unwritten block, so create
        // an empty block buffer to place the data
        fauxBytes = boost::make_shared<std::string>(maxObjectSizeInBytes, 0);
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
    return std::make_pair(offVec[index], fauxBytes);
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

NbdOperations::~NbdOperations() {
    delete nbdResp;
    nbdResp = nullptr;
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

    LOGDEBUG << "Will write 0x" << std::hex << length << " bytes to 0x" << offset
             << " handle 0x" << handle << std::dec;

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

        LOGDEBUG << "actualLen " << iLength << " bytesW " << amBytesWritten
                 << " length " << bytes->length();
        auto objBuf = (iLength == bytes->length()) ?
            bytes : boost::make_shared<std::string>(*bytes, amBytesWritten, iLength);

        // write an object
        boost::shared_ptr<int32_t> objLength = boost::make_shared<int32_t>(maxObjectSizeInBytes);
        boost::shared_ptr<apis::ObjectOffset> off(new apis::ObjectOffset());
        off->value = objectOff;

        // request id is 64 bit of handle + 32 bit of sequence Id
        handle_type reqId{handle, seqId};

        // For objects that we are only updating a part of, we need to perform
        // a Read-Modify-Write operation. To prevent a race condition we lock
        // that sector (4k object) for the duration of the operation and queue
        // other requests to that offset behind it. When the operation finishes
        // it pull the next op off the queue and enqueue it to QoS
        if (iLength != maxObjectSizeInBytes) {
            LOGDEBUG << "Will do read-modify-write for object size " << maxObjectSizeInBytes;
            // keep the data for the update to the first and last object in the response
            // so that we can apply the update to the object on read response
            resp->keepBufferForWrite(seqId, objectOff, objBuf);

            if (sector_type::QueueResult::FirstEntry ==
                    sector_map.queue_rmw(objectOff, reqId)) {
                amAsyncDataApi->getBlob(reqId,
                                        domainName,
                                        volumeName,
                                        blobName,
                                        objLength,
                                        off);
            } else {
                LOGDEBUG << "RMW queued: " << handle << ":" << seqId;
            }
        } else {
            // if we are here, we don't need to read this object first; will do write
            LOGDEBUG << "putBlob length " << maxObjectSizeInBytes << " offset " << curOffset
                     << " object offset " << objectOff
                     << " volume " << volumeName
                     << " reqId " << reqId.handle << ":" << reqId.seq;
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
        amBytesWritten += iLength;
        ++seqId;
    }
}

void
NbdOperations::getBlobResp(const Error &error,
                           handle_type& requestId,
                           const boost::shared_ptr<std::vector<boost::shared_ptr<std::string>>>& bufs,
                           int& length) {
    static auto empty_buffer = boost::make_shared<std::string>(0, 0x00);
    NbdResponseVector* resp = NULL;
    fds_int64_t handle = requestId.handle;
    uint32_t seqId = requestId.seq;
    fds_bool_t done = false;

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
        // this is a response for read during a write operation from NBD connector
        LOGDEBUG << "Write after read, handle 0x" << std::hex << handle << std::dec << " seqId " << seqId;

        // RMW only operates on a single buffer...
        auto& buf = !bufs->empty() ? bufs->front() : empty_buffer;

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
        } else {
            done = true;
        }
    } else {
        // this is response for read operation, add buf to the response list
        done = resp->handleReadResponse(*bufs, length, error);
    }

    if (done) {
        bool response_removed = false;
        {
            // nbd connector will free resp
            // remove from the wait list
            fds_mutex::scoped_lock l(respLock);
            response_removed = (1 == responses.erase(handle));
        }
        if (response_removed) {
            // we are done collecting responses for this handle, notify nbd connector
            nbdResp->readWriteResp(resp);
        } else {
            LOGNOTIFY << "Handle 0x" << std::hex << handle << std::dec
                      << " was missing from the response map!";
        }
    }
}

void
NbdOperations::updateBlobResp(const Error &error,
                              handle_type& requestId) {
    NbdResponseVector* resp = NULL;
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

    // Unblock other RMW on the same offset if they exist
    auto rmw = resp->wasRMW(seqId);
    if (rmw.first) {
        LOGDEBUG << "RMW finished offset: " << rmw.second;
        auto e = sector_map.pop_and_delete(rmw.second);
        if (e.first) {
            LOGDEBUG << "RMW queuing: " << e.second.handle << ":" << e.second.seq;
            boost::shared_ptr<int32_t> objLength =
                boost::make_shared<int32_t>(maxObjectSizeInBytes);
            boost::shared_ptr<apis::ObjectOffset> off(new apis::ObjectOffset());
            off->value = rmw.second;
            amAsyncDataApi->getBlob(e.second,
                                    domainName,
                                    volumeName,
                                    blobName,
                                    objLength,
                                    off);
        }
    }

    // update response vector
    fds_verify(resp);
    fds_bool_t done = resp->handleWriteResponse(seqId, error);
    if (done) {
        {
            // nbd connector will free resp
            // remove from the wait list
            fds_mutex::scoped_lock l(respLock);
            if (1 != responses.erase(handle)) {
                LOGERROR << "Handle 0x" << std::hex
                         << handle << std::dec
                         << " was missing from the response map!";
            }
        }

        // we are done collecting responses for this handle, notify nbd connector
        nbdResp->readWriteResp(resp);
    }
}

void
NbdOperations::shutdown()
{
    // If NbdConnection has not received the command to attach, volumeName
    // will still be NULL
    if (volumeName) {
        // Only close the volume if it's the last connection
        std::unique_lock<std::mutex> lk(assoc_map_lock);
        if (0 == --assoc_map[*volumeName]) {
            handle_type reqId{0, 0};
            amAsyncDataApi->detachVolume(reqId, domainName, volumeName);
        }
    }
    amAsyncDataApi.reset();
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
