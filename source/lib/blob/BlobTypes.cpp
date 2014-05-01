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

const_kv_iterator
BlobDescriptor::kvMetaBegin() const {
    return blobKvMeta.cbegin();
}

const_kv_iterator
BlobDescriptor::kvMetaEnd() const {
    return blobKvMeta.cend();
}

fds_uint64_t
BlobDescriptor::getBlobSize() const {
    return blobSize;
}

const std::string &
BlobDescriptor::getBlobName() const {
    return blobName;
}

void
BlobDescriptor::setBlobName(const std::string &name) {
    blobName = name;
}

void
BlobDescriptor::setBlobSize(fds_uint64_t size) {
    blobSize = size;
}

void
BlobDescriptor::addKvMeta(const std::string &key,
                          const std::string &value) {
    blobKvMeta[key] = value;
}

}  // namespace fds
