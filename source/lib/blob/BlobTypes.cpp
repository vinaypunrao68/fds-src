/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <blob/BlobTypes.h>
#include <string>
#include <utility>
#include <fds_uuid.h>

namespace fds {

// FIXME(DAC): We really shouldn't have a constructor that leaves most fields uninitialized.
BlobDescriptor::BlobDescriptor()
        : blobSize(0) {
}

BlobDescriptor::~BlobDescriptor() {
}

BlobDescriptor::BlobDescriptor(std::string const& blobName,
                               blob_version_t const blobVersion,
                               fds_volid_t const volumeUuid,
                               fds_uint64_t const blobSize,
                               BlobKeyValue const& blobKvMeta)
        : blobName(blobName),
          blobVersion(blobVersion),
          volumeUuid(volumeUuid),
          blobSize(blobSize),
          blobKvMeta(blobKvMeta) {
}

BlobDescriptor::BlobDescriptor(const BlobDescriptor::ptr &blobDesc)
        : BlobDescriptor(*blobDesc) {
}

BlobDescriptor::BlobDescriptor(const BlobDescriptor &blobDesc)
        : BlobDescriptor(blobDesc.blobName,
                         blobDesc.blobVersion,
                         blobDesc.volumeUuid,
                         blobDesc.blobSize,
                         blobDesc.blobKvMeta) {
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

fds_volid_t
BlobDescriptor::getVolId() const {
    return volumeUuid;
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
BlobDescriptor::updateBlobSize(fds_uint64_t size) {
    blobSize += size;
}

void
BlobDescriptor::setVolId(fds_volid_t volId) {
    volumeUuid = volId;
}

void
BlobDescriptor::addKvMeta(const std::string &key,
                          const std::string &value) {
    blobKvMeta[key] = value;
}

std::ostream&
operator<<(std::ostream& out, const BlobDescriptor& blobDesc) {
    out << "Blob " << blobDesc.blobName << " size " << blobDesc.blobSize
        << " volume " << std::hex << blobDesc.volumeUuid << std::dec;
    return out;
}

std::ostream&
operator<<(std::ostream& out, const BlobOffsetPair& blobOffset) {
    out << "Blob " << blobOffset.first << " offset " << blobOffset.second;
    return out;
}

std::ostream&
operator<<(std::ostream& out, const BlobTxId& txId) {
    return out << txId.txId;
}

}  // namespace fds
