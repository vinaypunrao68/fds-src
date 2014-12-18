/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_BLOB_BLOBTYPES_H_
#define SOURCE_INCLUDE_BLOB_BLOBTYPES_H_

#include <fds_types.h>
#include <string>
#include <list>
#include <unordered_map>
#include <utility>

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
    BlobDescriptor(std::string const& blobName,
                   blob_version_t const blobVersion,
                   fds_volid_t const volumeUuid,
                   fds_uint64_t const blobSize,
                   BlobKeyValue const& blobKvMeta);
    BlobDescriptor();
    ~BlobDescriptor();
    typedef boost::shared_ptr<BlobDescriptor> ptr;
    explicit BlobDescriptor(const BlobDescriptor::ptr &blobDesc);
    explicit BlobDescriptor(const BlobDescriptor &blobDesc);

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
    /**
     * Returns the volume ID
     */
    fds_volid_t getVolId() const;

    // TODO(Andrew): May want to remove for a single
    // func to set everything from FDSP?
    void setBlobName(const std::string &name);
    void setBlobSize(fds_uint64_t size);
    // FIXME(DAC): This is not updating, this is adding.
    void updateBlobSize(fds_uint64_t size);
    void setVolId(fds_volid_t volId);
    /// Adds key-value metadata, overwrite any old key
    void addKvMeta(const std::string &key,
                   const std::string &value);

    friend std::ostream& operator<<(std::ostream& out,
                                    const BlobDescriptor& blobDesc);
};

/**
 * Describes an aligned offset within a blob.
 */
class BlobOffsetPair : public std::pair<std::string, fds_uint64_t> {
  public:
    typedef boost::shared_ptr<BlobOffsetPair> ptr;
    typedef boost::shared_ptr<const BlobOffsetPair> const_ptr;

    BlobOffsetPair(const std::string &a, const fds_uint64_t &b)
            : std::pair<std::string, fds_uint64_t>(a, b) {
    }

    std::string toString() const {
        return first + std::to_string(second);
    }

    friend std::ostream& operator<<(std::ostream& out,
                                    const BlobOffsetPair& blobOffset);
};

class BlobOffsetPairHash {
  public:
    size_t operator()(const BlobOffsetPair &blobOffset) const {
        return std::hash<std::string>()(blobOffset.toString());
    }
};

/**
 * Describes a specific blob transaction
 */
class BlobTxId {
  private:
    fds_uint64_t txId;

  public:
    /// Creates a new blob transaction ID with invalid value
    BlobTxId();
    /// Creates a new blob transaction with a specific value
    explicit BlobTxId(fds_uint64_t givenId);
    explicit BlobTxId(const BlobTxId &rhs);
    ~BlobTxId();
    typedef boost::shared_ptr<BlobTxId> ptr;
    typedef boost::shared_ptr<const BlobTxId> const_ptr;

    static const fds_uint64_t txIdInvalid = 0;

    BlobTxId& operator=(const BlobTxId& rhs);
    fds_bool_t operator==(const BlobTxId& rhs) const;
    fds_bool_t operator!=(const BlobTxId& rhs) const;
    friend std::ostream& operator<<(std::ostream& out, const BlobTxId& txId);

    fds_uint64_t getValue() const;
};

/// Provides hashing function for trans id
class BlobTxIdHash {
  public:
    size_t operator()(const BlobTxId &id) const {
        return std::hash<fds_uint64_t>()(id.getValue());
    }
};

class BlobTxIdPtrHash {
  public:
    size_t operator()(const BlobTxId::ptr id) const {
        return BlobTxIdHash()(*id);
    }
};

static const BlobTxId blobTxIdInvalid(BlobTxId::txIdInvalid);

std::ostream& operator<<(std::ostream& out, const BlobTxId& txId);

/**
 * List structure containing const blob transaction ID ptrs
 */
class BlobTxList : public std::list<BlobTxId::const_ptr> {
  public:
    typedef boost::shared_ptr<BlobTxList> ptr;
    typedef boost::shared_ptr<const BlobTxList> const_ptr;
};

}  // namespace fds

namespace std {
template<>
struct hash<fds::BlobTxId> {
    std::size_t operator()(const fds::BlobTxId & key) const {
        return std::hash<fds_uint64_t>()(key.getValue());
    }
};
} /* namespace std */

#endif  // SOURCE_INCLUDE_BLOB_BLOBTYPES_H_
