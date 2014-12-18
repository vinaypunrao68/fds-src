/**
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <string>
#include <map>
#include <NbdOperations.h>

namespace fds {

NbdOperations::NbdOperations(NbdOperationsResponseIface* respIface)
        : amAsyncDataApi(std::make_shared<AmAsyncDataApi>(this)),
          nbdResp(respIface),
          domainName(new std::string("TestDomain")),
          blobName(new std::string("BlockBlob")),
          emptyMeta(new std::map<std::string, std::string>()),
          blobMode(new fds_int32_t(0)) {
}

NbdOperations::~NbdOperations() {
}

void
NbdOperations::read(boost::shared_ptr<std::string>& volumeName,
                    fds_uint32_t maxObjectSizeInBytes,
                    fds_uint32_t length,
                    fds_uint64_t offset,
                    fds_int64_t handle) {
    // calculate how many object we will get from AM
    fds_uint32_t objCount = length / maxObjectSizeInBytes;
    if ((length % maxObjectSizeInBytes) != 0) {
        ++objCount;
    }

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
        if ((seqId == 0) && (iOff != 0)) {
             LOGNORMAL << "Un-aligned read, starts at offset " << offset
                       << " iOff " << iOff << " length " << length;
             // we cannot read from AM the part of the object not from start
             iLength = maxObjectSizeInBytes;
        }

        boost::shared_ptr<int32_t> blobLength = boost::make_shared<int32_t>(iLength);
        boost::shared_ptr<apis::ObjectOffset> off(new apis::ObjectOffset());
        off->value = objectOff;

        boost::shared_ptr<apis::RequestId> reqId(
            boost::make_shared<apis::RequestId>());
        // request id is 64 bit of handle + 32 bit of sequence Id
        reqId->id = std::to_string(handle) + ":" + std::to_string(seqId);

        // blob name for block?
        LOGDEBUG << "getBlob length " << iLength << " offset " << curOffset
                 << " object offset " << objectOff
                 << " volume " << volumeName << " reqId " << reqId->id;
        try {
            amAsyncDataApi->getBlob(reqId,
                                    domainName,
                                    volumeName,
                                    blobName,
                                    blobLength,
                                    off);
            amBytesRead += actualLength;
            ++seqId;
        } catch(apis::ApiException fdsE) {
           // return error
           resp->setError(ERR_DISK_READ_FAILED);
           {
               // nbd connector will free resp
               // remove from the wait list
               fds_mutex::scoped_lock l(respLock);
               responses.erase(handle);
            }
            // we are done collecting responses for this handle, notify nbd connector
            nbdResp->readWriteResp(ERR_DISK_READ_FAILED, handle, resp);
            return;
        }
    }
}


void
NbdOperations::write(boost::shared_ptr<std::string>& volumeName,
                     fds_uint32_t maxObjectSizeInBytes,
                     boost::shared_ptr<std::string>& bytes,
                     fds_uint32_t length,
                     fds_uint64_t offset,
                     fds_int64_t handle) {
    // calculate how many object we will put
    fds_uint32_t objCount = length / maxObjectSizeInBytes;
    if ((length % maxObjectSizeInBytes) != 0) {
        ++objCount;
    }

    LOGDEBUG << "Will write " << length << " bytes " << offset
             << " offset for volume " << *volumeName
             << " object count " << objCount
             << " handle " << handle
             << " max object size " << maxObjectSizeInBytes << " bytes";

    // we will wait for responses
    NbdResponseVector* resp = new NbdResponseVector(handle,
                                                    NbdResponseVector::WRITE,
                                                    offset, length,
                                                    maxObjectSizeInBytes, objCount);

    {   // add response that we will fill in with data
        fds_mutex::scoped_lock l(respLock);
        fds_verify(responses.count(handle) == 0);
        responses[handle] = resp;
    }

    fds_uint32_t amBytesWritten = 0;
    fds_int32_t seqId = 0;
    while (amBytesWritten < length) {
        fds_uint64_t curOffset = offset + amBytesWritten;
        fds_uint64_t objectOff = curOffset / maxObjectSizeInBytes;
        fds_uint32_t iOff = curOffset % maxObjectSizeInBytes;
        fds_uint32_t iLength = length - amBytesWritten;
        if (iLength > maxObjectSizeInBytes) {
            iLength = maxObjectSizeInBytes - iOff;
        }

        // see if we need to read-modify-write
        if ((iOff != 0) || (iLength != maxObjectSizeInBytes)) {
            // we need to read the whole object first
            LOGNORMAL << "Will do read-modify-write for object size " << iLength;
            fds_verify(false);  // implement this
            ++seqId;
            continue;
        }

        // write an object
        boost::shared_ptr<int32_t> objLength = boost::make_shared<int32_t>(iLength);
        boost::shared_ptr<apis::ObjectOffset> off(new apis::ObjectOffset());
        off->value = objectOff;

        boost::shared_ptr<apis::RequestId> reqId(
            boost::make_shared<apis::RequestId>());
        // request id is 64 bit of handle + 32 bit of sequence Id
        reqId->id = std::to_string(handle) + ":" + std::to_string(seqId);

        // blob name for block?
        LOGDEBUG << "putBlob length " << iLength << " offset " << curOffset
                 << " object offset " << objectOff
                 << " volume " << volumeName << " reqId " << reqId->id;

        // write to AM
        try {
            amAsyncDataApi->updateBlobOnce(reqId,
                                           domainName,
                                           volumeName,
                                           blobName,
                                           blobMode,
                                           bytes,
                                           objLength,
                                           off,
                                           emptyMeta);
            amBytesWritten += iLength;
            ++seqId;
        } catch(apis::ApiException fdsE) {
            // return error
            resp->setError(ERR_DISK_WRITE_FAILED);
            {
                 // nbd connector will free resp
                 // remove from the wait list
                 fds_mutex::scoped_lock l(respLock);
                 responses.erase(handle);
             }
             // we are done collecting responses for this handle, notify nbd connector
             nbdResp->readWriteResp(ERR_DISK_WRITE_FAILED, handle, resp);
             return;
        }
    }
}

void
NbdOperations::getBlobResp(const Error &error,
                           boost::shared_ptr<apis::RequestId>& requestId,
                           boost::shared_ptr<std::string> buf,
                           fds_uint32_t& length) {
    NbdResponseVector* resp = NULL;
    fds_int64_t handle = 0;
    fds_int32_t seqId = 0;
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

    // add buffer to the response list
    fds_verify(resp);
    fds_bool_t done = resp->handleReadResponse(buf, length, seqId, error);
    if (done) {
        {
            // nbd connector will free resp
            // remove from the wait list
            fds_mutex::scoped_lock l(respLock);
            responses.erase(handle);
        }
        // we are done collecting responses for this handle, notify nbd connector
        nbdResp->readWriteResp(error, handle, resp);
    }
}

void
NbdOperations::updateBlobOnceResp(const Error &error,
                                  boost::shared_ptr<apis::RequestId>& requestId) {
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
        nbdResp->readWriteResp(error, handle, resp);
    }
}

void
NbdOperations::parseRequestId(boost::shared_ptr<apis::RequestId>& requestId,
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
