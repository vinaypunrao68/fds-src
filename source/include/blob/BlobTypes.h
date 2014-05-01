/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_BLOB_BLOBTYPES_H_
#define SOURCE_INCLUDE_BLOB_BLOBTYPES_H_

#include <fds_types.h>
#include <string>
#include <unordered_map>

namespace fds {

/**
 * Describes a blob's key-value metadata pairs
 * The key and value types are both meant to be strings.
 */
typedef std::unordered_map<std::string, std::string> BlobKeyValue;
typedef BlobKeyValue::const_iterator const_kv_iterator;

/**
 * Descriptor for a blob. This structure is meant to
 * be useable by various services and describe a blob
 * and its associated metadata.
 */
class BlobDescriptor {
  private:
    std::string    blobName;
    blob_version_t blobVersion;
    fds_volid_t    volumeUuid;
    fds_uint64_t   blobSize;

    BlobKeyValue   blobKvMeta;

  public:
    BlobDescriptor();
    ~BlobDescriptor();
    typedef boost::shared_ptr<BlobDescriptor> ptr;

    /**
     * Returns const iterator to key value metadata
     */
    const_kv_iterator kvMetaBegin() const;
    /**
     * Returns const iterator to key value metadata
     */
    const_kv_iterator kvMetaEnd() const;

    /**
     * Returns the size of the blob in bytes
     */
    fds_uint64_t getBlobSize() const;
    /**
     * Returns the blob name
     */
    const std::string &getBlobName() const;

    // TODO(Andrew): May want to remove for a single
    // func to set everything from FDSP?
    void setBlobName(const std::string &name);
    void setBlobSize(fds_uint64_t size);
    /// Adds key-value metadata, overwrite any old key
    void addKvMeta(const std::string &key,
                   const std::string &value);
};
}  // namespace fds

#endif  // SOURCE_INCLUDE_BLOB_BLOBTYPES_H_
