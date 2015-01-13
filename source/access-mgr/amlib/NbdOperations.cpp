/**
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include "NbdOperations.h"

#include <algorithm>
#include <map>
#include <string>
#include <utility>

namespace fds {

fds_bool_t
NbdResponseVector::handleReadResponse(boost::shared_ptr<std::string> retBuf,
                                      fds_uint32_t len,
                                      fds_uint32_t seqId,
                                      const Error& err) {
    fds_verify(operation == READ);
    fds_verify(seqId < bufVec.size());

    if (!err.ok() && (err != ERR_BLOB_OFFSET_INVALID) &&
                     (err != ERR_BLOB_NOT_FOUND)) {
        opError = err;
        return true;
    }

    fds_uint32_t iOff = offset % maxObjectSizeInBytes;
    fds_uint32_t firstObjectLength = std::min(length,
                                              maxObjectSizeInBytes - iOff);
    fds_uint32_t iLength = maxObjectSizeInBytes;

    // The first and last buffers may not be an entire block
    if (seqId == 0) {
        iLength = firstObjectLength;
    } else if (seqId == (objCount - 1)) {
        iLength = length - firstObjectLength - (objCount-2) * maxObjectSizeInBytes;
    }

    if ((err == ERR_BLOB_OFFSET_INVALID) || (err == ERR_BLOB_NOT_FOUND)) {
        // we tried to read unwritten block, fill in zeros
        bufVec[seqId] = boost::make_shared<std::string>(iLength, 0);
    } else {
        // Else grab the portion of the string that we need, or the entire
        // thing
        bufVec[seqId] = (iLength == maxObjectSizeInBytes) ?
            retBuf :
            boost::make_shared<std::string>(retBuf->data() + (seqId == 0 ? iOff : 0), iLength);
    }
    fds_uint32_t doneCnt = atomic_fetch_add(&doneCount, (fds_uint32_t)1);
    return ((doneCnt + 1) == objCount);
}

std::pair<fds_uint64_t, boost::shared_ptr<std::string>>
NbdResponseVector::handleRMWResponse(boost::shared_ptr<std::string> retBuf,
                                 fds_uint32_t len,
                                 fds_uint32_t seqId,
                                 const Error& err) {
    fds_verify(operation == WRITE);
    fds_uint32_t index = (seqId == 0) ? 0 : 1;
    if (!err.ok() && (err != ERR_BLOB_OFFSET_INVALID) &&
                     (err != ERR_BLOB_NOT_FOUND)) {
        opError = err;
    } else {
        fds_uint32_t iOff = (seqId == 0) ? offset % maxObjectSizeInBytes : 0;
        boost::shared_ptr<std::string> writeBytes = bufVec[index];

        boost::shared_ptr<std::string> fauxBytes;
        if ((err == ERR_BLOB_OFFSET_INVALID) ||
            (err == ERR_BLOB_NOT_FOUND)) {
            // we tried to read unwritten block, so create
            // an empty block buffer to place the data
            LOGTRACE << "Creating new object and writing at offset: " << iOff << " for length: " << writeBytes->length();  // NOLINT
            fauxBytes = boost::make_shared<std::string>(maxObjectSizeInBytes, 0);
            fauxBytes->replace(iOff, writeBytes->length(),
                               writeBytes->c_str(), writeBytes->length());
        } else {
            fds_verify(len == maxObjectSizeInBytes);
            // Need to copy retBut into a modifiable buffer since retBuf is owned
            // by AM and should not be modified here.
            // TODO(Andrew): Make retBuf a const
            LOGTRACE << "Updating object at offset: " << iOff << " for length: " << writeBytes->length();  // NOLINT
            fauxBytes = boost::make_shared<std::string>(retBuf->c_str(), retBuf->length());
            fauxBytes->replace(iOff, writeBytes->length(),
                               writeBytes->c_str(), writeBytes->length());
        }
        return std::make_pair(offVec[index], fauxBytes);
    }
    return std::make_pair(offVec[index], boost::shared_ptr<std::string>());
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
NbdOperations::init(boost::shared_ptr<std::string> vol_name, fds_uint32_t _maxObjectSizeInBytes) {
    amAsyncDataApi.reset(new AmAsyncDataApi(shared_from_this()));
    volumeName = vol_name;
    maxObjectSizeInBytes = _maxObjectSizeInBytes;
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
    // calculate how many object we will get from AM
    fds_uint32_t objCount = getObjectCount(length, offset);

    LOGDEBUG << "Will read " << length << " bytes " << offset
             << " offset for volume " << *volumeName
             << " object count " << objCount
             << " handle " << handle
             << " max object size " << maxObjectSizeInBytes << " bytes";

    // we will wait for responses
    NbdResponseVector* resp = new NbdResponseVector(handle,
                                                    NbdResponseVector::READ,
                                                    offset, length, maxObjectSizeInBytes,
                                                    objCount);

    {   // add response that we will fill in with data
        fds_mutex::scoped_lock l(respLock);
        fds_verify(responses.count(handle) == 0);
        responses[handle] = resp;
    }

    // break down request into max obj size chunks and send to AM
    fds_uint32_t amBytesRead = 0;
    fds_int32_t seqId = 0;
    while (amBytesRead < length) {
        fds_uint64_t curOffset = offset + amBytesRead;
        fds_uint64_t objectOff = curOffset / maxObjectSizeInBytes;
        fds_uint32_t iOff = curOffset % maxObjectSizeInBytes;
        // iLength is length of object we will read from AM
        fds_uint32_t iLength = length - amBytesRead;
        // three cases here for the length of the first object
        // 1) Aligned: min(length, maxObjectSizeInBytes)
        // 2) Un-aligned and data spills over the next object
        //    maxObjectSizeInBytes - iOff
        // 3) Un-aligned otherwise: length
        if (iLength >= maxObjectSizeInBytes) {
            iLength = maxObjectSizeInBytes - iOff;
        } else if ((seqId == 0) && (iOff != 0)) {
            if (length > (maxObjectSizeInBytes - iOff)) {
                iLength = maxObjectSizeInBytes - iOff;
            }
        }
        fds_uint32_t actualLength = iLength;
        // if the first read does not start at the beginning of object
        // (un-aligned), then we will read the whole first object from AM
        // and then return requested part for it to NBD connector
        if ((seqId == 0) && (iOff != 0)) {
             LOGNORMAL << "Un-aligned read, starts at offset " << offset
                       << " iOff " << iOff << " length " << length;
             // we cannot read from AM the part of the object not from start
             iLength = maxObjectSizeInBytes;
        }

        // we will read object of length iLength from AM
        boost::shared_ptr<int32_t> blobLength = boost::make_shared<int32_t>(iLength);
        boost::shared_ptr<apis::ObjectOffset> off(new apis::ObjectOffset());
        off->value = objectOff;

        // we encode handle and sequence ID into request ID; handle is needed
        // to reply back to NBD connector and sequence ID is needed to assemble
        // all object we read back into read request from NBD connector
        handle_type reqId(boost::make_shared<apis::RequestId>());
        // request id is 64 bit of handle + 32 bit of sequence Id
        reqId->id = std::to_string(handle) + ":" + std::to_string(seqId);

        // blob name for block?
        LOGDEBUG << "getBlob length " << iLength << " offset " << curOffset
                 << " object offset " << objectOff
                 << " volume " << volumeName << " reqId " << reqId->id;
        amAsyncDataApi->getBlob(reqId,
                                domainName,
                                volumeName,
                                blobName,
                                blobLength,
                                off);
        amBytesRead += actualLength;
        ++seqId;
    }
}


void
NbdOperations::write(boost::shared_ptr<std::string>& bytes,
                     fds_uint32_t length,
                     fds_uint64_t offset,
                     fds_int64_t handle) {
    fds_assert(amAsyncDataApi);
    // calculate how many FDS objects we will write
    fds_uint32_t objCount = getObjectCount(length, offset);

    LOGDEBUG << "Will write " << length << " bytes " << offset
             << " offset for volume " << *volumeName
             << " object count " << objCount
             << " handle " << handle
             << " max object size " << maxObjectSizeInBytes << " bytes";

    // we will wait for write response for all objects we chunk this request into
    NbdResponseVector* resp = new NbdResponseVector(handle,
                                                    NbdResponseVector::WRITE,
                                                    offset, length,
                                                    maxObjectSizeInBytes, objCount);

    {   // add response that we will fill in with data
        fds_mutex::scoped_lock l(respLock);
        fds_verify(responses.count(handle) == 0);
        responses[handle] = resp;
    }

    size_t amBytesWritten = 0;
    fds_int32_t seqId = 0;
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
        handle_type reqId(boost::make_shared<apis::RequestId>());
        reqId->id = std::to_string(handle) + ":" + std::to_string(seqId);

        // For objects that we are only updating a part of, we need to perform
        // a Read-Modify-Write operation. To prevent a race condition we lock
        // that sector (4k object) for the duration of the operation and queue
        // other requests to that offset behind it. When the operation finishes
        // it pull the next op off the queue and enqueue it to QoS
        if (iLength != maxObjectSizeInBytes) {
            LOGNORMAL << "Will do read-modify-write for object size " << maxObjectSizeInBytes;
            // keep the data for the update to the first and last object in the response
            // so that we can apply the update to the object on read response
            resp->keepBufferForWrite(seqId, objectOff, objBuf);

            if (sector_type::QueueResult::FirstEntry ==
                    sector_map.queue_rmw(objectOff, {handle, seqId})) {
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
                     << " volume " << volumeName << " reqId " << reqId->id;
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
                           boost::shared_ptr<std::string> buf,
                           fds_uint32_t& length) {
    NbdResponseVector* resp = NULL;
    fds_int64_t handle = 0;
    fds_int32_t seqId = 0;
    fds_bool_t done = false;
    parseRequestId(requestId, &handle, &seqId);

    LOGDEBUG << "Reponse for getBlob, " << length << " bytes "
             << error << ", handle " << handle
             << " seqId " << seqId << " ( "
             << requestId->id << " )";

    {
        fds_mutex::scoped_lock l(respLock);
        // if we are not waiting for this response, we probably already
        // returned an error
        if (responses.count(handle) == 0) {
            LOGWARN << "Not waiting for response for handle " << handle
                    << ", check if we returned an error";
            return;
        }
        // get response
        resp = responses[handle];
    }

    fds_verify(resp);
    if (!resp->isRead()) {
        // this is a response for read during a write operation from NBD connector
        LOGDEBUG << "Write after read, handle " << handle << " seqId " << seqId;

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
        done = resp->handleReadResponse(buf, length, seqId, error);
    }

    if (done) {
        {
            // nbd connector will free resp
            // remove from the wait list
            fds_mutex::scoped_lock l(respLock);
            responses.erase(handle);
        }

        // we are done collecting responses for this handle, notify nbd connector
        nbdResp->readWriteResp(resp);
    }
}

void
NbdOperations::updateBlobResp(const Error &error,
                              handle_type& requestId) {
    NbdResponseVector* resp = NULL;
    fds_int64_t handle = 0;
    fds_int32_t seqId = 0;
    parseRequestId(requestId, &handle, &seqId);

    LOGDEBUG << "Reponse for updateBlobOnce, "
             << error << ", handle " << handle
             << " seqId " << seqId << " ( "
             << requestId->id << " )";

    {
        fds_mutex::scoped_lock l(respLock);
        // if we are not waiting for this response, we probably already
        // returned an error
        if (responses.count(handle) == 0) {
            LOGWARN << "Not waiting for response for handle " << handle
                    << ", check if we returned an error";
            return;
        }
        // get response
        resp = responses[handle];
    }

    // Unblock other RMW on the same offset if they exist
    auto rmw = resp->wasRMW(seqId);
    if (rmw.first) {
        LOGDEBUG << "RMW finished offset: " << rmw.second;
        auto e = sector_map.pop_and_delete(rmw.second);
        if (e.first) {
            LOGDEBUG << "RMW queuing: " << e.second.handle << ":" << e.second.seq;
            handle_type reqId(boost::make_shared<apis::RequestId>());
            reqId->id = std::to_string(e.second.handle) + ":" + std::to_string(e.second.seq);
            boost::shared_ptr<int32_t> objLength =
                boost::make_shared<int32_t>(maxObjectSizeInBytes);
            boost::shared_ptr<apis::ObjectOffset> off(new apis::ObjectOffset());
            off->value = rmw.second;
            amAsyncDataApi->getBlob(reqId,
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
            responses.erase(handle);
        }

        // we are done collecting responses for this handle, notify nbd connector
        nbdResp->readWriteResp(resp);
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

void
NbdOperations::parseRequestId(handle_type& requestId,
                              fds_int64_t* handle,
                              fds_int32_t* seqId) {
    std::string delim(":");
    size_t start = 0;
    size_t end = (requestId->id).find(delim, start);
    fds_verify(end != std::string::npos);
    std::string handleStr = (requestId->id).substr(start,
                                                   end - start);
    std::string seqIdStr = (requestId->id).substr(end + delim.size());

    // return
    *handle = std::stoll(handleStr);
    *seqId = std::stol(seqIdStr);
}

}  // namespace fds
