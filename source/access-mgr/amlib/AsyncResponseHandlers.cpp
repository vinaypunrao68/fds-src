/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#include "AsyncResponseHandlers.h"

namespace fds
{

boost::shared_ptr<apis::BlobDescriptor>
transform_descriptor(boost::shared_ptr<BlobDescriptor> descriptor) {
    auto retBlobDesc = boost::make_shared<apis::BlobDescriptor>();
    retBlobDesc->name = descriptor->getBlobName();
    retBlobDesc->byteCount = descriptor->getBlobSize();

    for (const_kv_iterator it = descriptor->kvMetaBegin();
         it != descriptor->kvMetaEnd();
         ++it) {
        retBlobDesc->metadata[it->first] = it->second;
    }
    return retBlobDesc;
}


// TODO(bszmyd): Tue 16 Dec 2014 08:06:47 PM MST
// Unfortunately we have to transform meta-data received
// from the DataManager. We should fix that.
template<>
void StatBlobHandler::process() {
    boost::shared_ptr<apis::BlobDescriptor> retBlobDesc = error.ok() ?
        transform_descriptor(blobDesc) :
        nullptr;
    respApi->statBlobResp(error, requestId, retBlobDesc);
}

template<>
void GetBlobWithMetadataHandler::process() {
    boost::shared_ptr<apis::BlobDescriptor> retBlobDesc = error.ok() ?
        transform_descriptor(blobDesc) :
        nullptr;
    respApi->getBlobWithMetaResp(error, requestId, returnBuffer, returnSize, retBlobDesc);
}

template<>
void VolumeStatusHandler::process() {
    boost::shared_ptr<apis::VolumeStatus> volume_status;
    if (error.ok()) {
        volume_status = boost::make_shared<apis::VolumeStatus>();
        volume_status->blobCount = volumeMetaData.blobCount;
        volume_status->currentUsageInBytes = volumeMetaData.size;
    }
    respApi->volumeStatusResp(error, requestId, volume_status);
}

}  // namespace fds
