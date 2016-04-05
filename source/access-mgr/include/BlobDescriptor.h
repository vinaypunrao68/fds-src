/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_BLOBDESCRIPTOR_H_
#define SOURCE_ACCESS_MGR_INCLUDE_BLOBDESCRIPTOR_H_

#include <map>
#include <string>
#include <ostream>

#include <boost/shared_ptr.hpp>

namespace fds
{

/**
 * Descriptor for a blob. This structure is meant to
 * be useable by various services and describe a blob
 * and its associated metadata.
 */
struct BlobDescriptor {
    /**
     * Describes a blob's key-value metadata pairs
     * The key and value types are both meant to be strings.
     */
    using BlobKeyValue = std::map<std::string, std::string>;
    using const_kv_iterator = BlobKeyValue::const_iterator;
    using ptr = boost::shared_ptr<BlobDescriptor>;

    BlobDescriptor() = default;
    BlobDescriptor(std::string const& _blobName,
                   uint64_t const _volumeUuid,
                   size_t const _blobSize,
                   BlobKeyValue const& _blobKvMeta)
        : blobName(_blobName),
          volumeUuid(_volumeUuid),
          blobSize(_blobSize),
          blobKvMeta(_blobKvMeta)
    { }

    explicit BlobDescriptor(BlobDescriptor const& rhs) = default;
    explicit BlobDescriptor(BlobDescriptor&& rhs) = default;
    BlobDescriptor& operator=(BlobDescriptor const& rhs) = default;
    BlobDescriptor& operator=(BlobDescriptor&& rhs) = default;
    ~BlobDescriptor() = default;

    /**
     * Returns const iterator to key value metadata
     */
    const_kv_iterator kvMetaBegin() const
    { return blobKvMeta.cbegin(); }
    /**
     * Returns const iterator to key value metadata
     */
    const_kv_iterator kvMetaEnd() const
    { return blobKvMeta.cend(); }

    /**
     * Returns the size of the blob in bytes
     */
    size_t getBlobSize() const
    { return blobSize; }

    /**
     * Returns the blob name
     */
    const std::string &getBlobName() const
    { return blobName; }

    /**
     * Returns the volume ID
     */
    uint64_t getVolId() const
    { return volumeUuid; }

    // TODO(Andrew): May want to remove for a single
    // func to set everything from FDSP?
    void setBlobName(const std::string &name)
    { blobName = name; }

    void setBlobSize(size_t const size)
    { blobSize  = size; }

    // FIXME(DAC): This is not updating, this is adding.
    void updateBlobSize(size_t const size)
    { blobSize += size; }

    void setVolId(uint64_t const volId)
    { volumeUuid = volId; }

    /// Adds key-value metadata, overwrite any old key
    void addKvMeta(const std::string &key, const std::string &value)
    { blobKvMeta[key] = value; }

    friend std::ostream& operator<<(std::ostream& out, const BlobDescriptor& blobDesc);

  private:
    std::string    blobName;
    uint64_t    volumeUuid;
    size_t   blobSize;

    BlobKeyValue   blobKvMeta;
};

inline std::ostream&
operator<<(std::ostream& out, const BlobDescriptor& blobDesc) {
    return out << "Blob " << blobDesc.blobName << " size " << blobDesc.blobSize
        << " volume " << std::hex << blobDesc.volumeUuid << std::dec;
}


}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_BLOBDESCRIPTOR_H_
