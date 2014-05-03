/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <blob/BlobTypes.h>
#include <string>

namespace fds {

BlobDescriptor::BlobDescriptor() {
}

BlobDescriptor::~BlobDescriptor() {
}

fds_uint64_t
BlobDescriptor::getBlobSize() const {
    return blobSize;
}

void
BlobDescriptor::setBlobName(const std::string &name) {
    blobName = name;
}

void
BlobDescriptor::setBlobSize(fds_uint64_t size) {
    blobSize = size;
}

}  // namespace fds
