/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <am-engine/handlers/responsehandler.h>
#include <util/Log.h>
#include <util/timeutils.h>
#include <string>
#include <sstream>
#include <vector>
#define XCHECKSTATUS(status)             \
    if (status != FDSN_StatusOK) {       \
        LOGWARN << " status:" << status; \
        apis::ApiException e;            \
        e.errorCode = apis::ErrorCode::INTERNAL_SERVER_ERROR;   \
        e.message = toString(status);   \
        throw e;                        \
    }

namespace fds {

void ResponseHandler::call() {
    switch (type) {
        case HandlerType::WAITEDFOR:
            ready(); break;
        case HandlerType::IMMEDIATE:
            process() ; break;
        case HandlerType::QUEUED:
            fds_panic("QUEUED - not implemented yet");
            break;
    }
}

void ResponseHandler::ready() {
    task.done();
}

void ResponseHandler::wait() {
    task.await();
}

    bool ResponseHandler::waitFor(ulong timeout) {
    return task.await(timeout);
}

ResponseHandler::~ResponseHandler() {
}

//================================================================================

SimpleResponseHandler::SimpleResponseHandler() {
}

SimpleResponseHandler::SimpleResponseHandler(const std::string& name) : name(name) {
}

void SimpleResponseHandler::process() {
    LOGDEBUG << "processing callback for : " << name;
    XCHECKSTATUS(status);
}

SimpleResponseHandler::~SimpleResponseHandler() {
}

//================================================================================

void PutObjectResponseHandler::process() {
}

PutObjectResponseHandler::~PutObjectResponseHandler() {
}
//================================================================================

void PutObjectBlkResponseHandler::process() {
    if (status == FDSN_StatusOK) {
        ubdCallback(0);
    } else {
        // TODO(Andrew): For now, just pass -1 when something
        // we wrong
        ubdCallback(-1);
    }
}

PutObjectBlkResponseHandler::~PutObjectBlkResponseHandler() {
}
//================================================================================

void GetObjectResponseHandler::process() {
}

GetObjectResponseHandler::~GetObjectResponseHandler() {
}
//================================================================================

ListBucketResponseHandler::ListBucketResponseHandler(std::vector<apis::BlobDescriptor> & vecBlobs) : vecBlobs(vecBlobs) { //NOLINT
}

void ListBucketResponseHandler::process() {
    XCHECKSTATUS(status);
    for (int i = 0; i < contentsCount; i++) {
        apis::BlobDescriptor blob;
        blob.name = contents[i].objKey;
        blob.byteCount = contents[i].size;
        vecBlobs.push_back(blob);
    }
}

ListBucketResponseHandler::~ListBucketResponseHandler() {
    if (contents) delete[] contents;
    if (commonPrefixes) {
        // delete *commonPrefixes;
        // delete commonPrefixes;
    }
}

//================================================================================

BucketStatsResponseHandler::BucketStatsResponseHandler(
    apis::VolumeDescriptor& volumeDescriptor) : volumeDescriptor(volumeDescriptor) {
}

void BucketStatsResponseHandler::process() {
    XCHECKSTATUS(status);

    if (content_count == 0 || !contents) {
        LOGWARN << "response has no bucket data";
        status = FDSN_StatusErrorBucketNotExists;
        XCHECKSTATUS(status);
    }

    volumeDescriptor.name = contents[0].bucket_name;
    // volumeDescriptor.uuid = 10;
    volumeDescriptor.dateCreated = util::getTimeStampMillis();
    volumeDescriptor.policy.maxObjectSizeInBytes = 2097152;  // 2MB
}

BucketStatsResponseHandler::~BucketStatsResponseHandler() {
    if (contents) {
        delete[] contents;
    }
}

StatBlobResponseHandler::StatBlobResponseHandler(
    apis::BlobDescriptor& retVal) : retBlobDesc(retVal) {
}


void StatBlobResponseHandler::process() {
    XCHECKSTATUS(status);

    retBlobDesc.name      = blobDesc.getBlobName();
    retBlobDesc.byteCount = blobDesc.getBlobSize();

    for (const_kv_iterator it = blobDesc.kvMetaBegin();
         it != blobDesc.kvMetaEnd();
         it++) {
        retBlobDesc.metadata[it->first] = it->second;
    }
}

StatBlobResponseHandler::~StatBlobResponseHandler() {
}

StartBlobTxResponseHandler::StartBlobTxResponseHandler(
    apis::TxDescriptor& retVal) : retTxDesc(retVal) {
}

void
StartBlobTxResponseHandler::process() {
    XCHECKSTATUS(status);

    retTxDesc.txId = blobTxId.getValue();
}

StartBlobTxResponseHandler::~StartBlobTxResponseHandler() {
}

AttachVolumeResponseHandler::AttachVolumeResponseHandler() {
}

void
AttachVolumeResponseHandler::process() {
    XCHECKSTATUS(status);
}

AttachVolumeResponseHandler::~AttachVolumeResponseHandler() {
}

}  // namespace fds
