/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <blob/BlobTypes.h>
#include <string>
#include <fds_uuid.h>

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

BlobTxId::BlobTxId() {
    txId = fds_get_uuid64(get_uuid());
}

BlobTxId::~BlobTxId() {
}

fds_uint64_t
BlobTxId::getValue() const {
    return txId;
}

std::ostream&
operator<<(std::ostream& out, const BlobTxId& txId) {
    return out << txId.txId;
}

}  // namespace fds
