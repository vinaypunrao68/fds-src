/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#include "AsyncResponseHandlers.h"

namespace fds
{

// TODO(bszmyd): Tue 16 Dec 2014 08:06:47 PM MST
// Unfortunately we have to transform meta-data received
// from the DataManager. We should fix that.
template<>
void StatBlobHandler::process() {
    auto retBlobDesc = boost::make_shared<apis::BlobDescriptor>();
    retBlobDesc->name = blobDesc->getBlobName();
    retBlobDesc->byteCount = blobDesc->getBlobSize();

    for (const_kv_iterator it = blobDesc->kvMetaBegin();
         it != blobDesc->kvMetaEnd();
         ++it) {
        retBlobDesc->metadata[it->first] = it->second;
    }
    respApi->statBlobResp(error, requestId, retBlobDesc);
}

template<>
void GetBlobWithMetadataHandler::process() {
    auto retBlobDesc = boost::make_shared<apis::BlobDescriptor>();
    retBlobDesc->name = blobDesc->getBlobName();
    retBlobDesc->byteCount = blobDesc->getBlobSize();

    for (const_kv_iterator it = blobDesc->kvMetaBegin();
         it != blobDesc->kvMetaEnd();
         ++it) {
        retBlobDesc->metadata[it->first] = it->second;
    }
    respApi->getBlobWithMetaResp(error, requestId, returnBuffer, returnSize, retBlobDesc);
}

template<> void VolumeStatusHandler::process() {
    auto volumeStatus = boost::make_shared<apis::VolumeStatus>();
    volumeStatus->blobCount = volumeMetaData.blobCount;
    volumeStatus->currentUsageInBytes = volumeMetaData.size;
    respApi->volumeStatusResp(error, requestId, volumeStatus);
}

}  // namespace fds
