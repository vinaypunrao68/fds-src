/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMASYNCRESPONSEAPI_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMASYNCRESPONSEAPI_H_

#include <string>
#include <vector>

#include "fdsp/xdi_types.h"

namespace fpi = FDS_ProtocolInterface;

namespace fds {

struct VolumeDesc;

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

    ~BlobDescriptor() = default;
    explicit BlobDescriptor(const BlobDescriptor &blobDesc) = default;

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

struct RequestHandle {
    uint64_t handle;
    uint32_t seq;
};

/**
 * AM's data API that is exposed to XDI. This interface is the
 * basic data API that XDI and connectors are programmed to. A
 * pure virtual interface is exposed so that the implementation
 * of the responses can be overloaded (e.g., respond to XDI, respond
 * to unit test, etc...).
 */
class AmAsyncResponseApi {
  public:
    template <typename M> using sp = boost::shared_ptr<M>;
  
    typedef RequestHandle handle_type;
    typedef fpi::ErrorCode error_type;
    typedef int size_type;
    typedef sp<std::string> shared_buffer_type;
    typedef sp<std::vector<shared_buffer_type>> shared_buffer_array_type;
    typedef sp<BlobDescriptor> shared_descriptor_type;
    typedef sp<VolumeDesc> shared_vol_descriptor_type;
    typedef sp<std::vector<BlobDescriptor>> shared_descriptor_vec_type;
    typedef sp<std::vector<std::string>> shared_string_vec_type;
    typedef sp<FDS_ProtocolInterface::VolumeAccessMode> shared_vol_mode_type;
    typedef sp<apis::TxDescriptor> shared_tx_ctx_type;
    typedef sp<apis::VolumeStatus> shared_status_type;
    typedef sp<std::map<std::string, std::string>> shared_meta_type;

    virtual void attachVolumeResp(const error_type &error,
                                  handle_type const& requestId,
                                  shared_vol_descriptor_type& volDesc,
                                  shared_vol_mode_type& mode) = 0;
    virtual void detachVolumeResp(const error_type &error,
                                  handle_type const& requestId) = 0;
    virtual void startBlobTxResp(const error_type &error,
                                 handle_type const& requestId,
                                 shared_tx_ctx_type& txDesc) = 0;
    virtual void abortBlobTxResp(const error_type &error,
                                 handle_type const& requestId) = 0;
    virtual void commitBlobTxResp(const error_type &error,
                                  handle_type const& requestId) = 0;

    virtual void updateBlobResp(const error_type &error,
                                handle_type const& requestId) = 0;
    virtual void updateBlobOnceResp(const error_type &error,
                                    handle_type const& requestId) = 0;
    virtual void updateMetadataResp(const error_type &error,
                                    handle_type const& requestId) = 0;
    virtual void renameBlobResp(const error_type &error,
                                handle_type const& requestId,
                                shared_descriptor_type& blobDesc) = 0;
    virtual void deleteBlobResp(const error_type &error,
                                handle_type const& requestId) = 0;

    virtual void statBlobResp(const error_type &error,
                              handle_type const& requestId,
                              shared_descriptor_type& blobDesc) = 0;
    virtual void volumeStatusResp(const error_type &error,
                                  handle_type const& requestId,
                                  shared_status_type& volumeStatus) = 0;
    virtual void volumeContentsResp(
        const error_type &error,
        handle_type const& requestId,
        shared_descriptor_vec_type& volContents,
        shared_string_vec_type& skippedPrefixes) = 0;

    virtual void setVolumeMetadataResp(const error_type &error,
                                       handle_type const& requestId) = 0;

    virtual void getVolumeMetadataResp(const error_type &error,
                                       handle_type const& requestId,
                                       shared_meta_type& metadata) = 0;

    virtual void getBlobResp(const error_type &error,
                             handle_type const& requestId,
                             shared_buffer_array_type const& buf,
                             size_type& length) = 0;
    virtual void getBlobWithMetaResp(const error_type &error,
                                     handle_type const& requestId,
                                     shared_buffer_array_type const& buf,
                                     size_type& length,
                                     shared_descriptor_type& blobDesc) = 0;
};

inline std::ostream&
operator<<(std::ostream& out, const BlobDescriptor& blobDesc) {
    return out << "Blob " << blobDesc.blobName << " size " << blobDesc.blobSize
        << " volume " << std::hex << blobDesc.volumeUuid << std::dec;
}

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMASYNCRESPONSEAPI_H_
