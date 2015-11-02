/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMASYNCRESPONSEAPI_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMASYNCRESPONSEAPI_H_

#include <string>
#include <vector>
#include "fdsp/common_types.h"
#include "fdsp/xdi_types.h"

namespace fpi = FDS_ProtocolInterface;

namespace fds {

struct BlobDescriptor;
struct VolumeDesc;

/**
 * AM's data API that is exposed to XDI. This interface is the
 * basic data API that XDI and connectors are programmed to. A
 * pure virtual interface is exposed so that the implementation
 * of the responses can be overloaded (e.g., respond to XDI, respond
 * to unit test, etc...).
 */
template<typename H>
class AmAsyncResponseApi {
    using BlobDescriptor = ::FDS_ProtocolInterface::BlobDescriptor;
  public:
    template <typename M> using sp = boost::shared_ptr<M>;
  
    typedef H handle_type;
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
                                  handle_type& requestId,
                                  shared_vol_descriptor_type& volDesc,
                                  shared_vol_mode_type& mode) = 0;
    virtual void detachVolumeResp(const error_type &error,
                                  handle_type& requestId) = 0;
    virtual void startBlobTxResp(const error_type &error,
                                 handle_type& requestId,
                                 shared_tx_ctx_type& txDesc) = 0;
    virtual void abortBlobTxResp(const error_type &error,
                                 handle_type& requestId) = 0;
    virtual void commitBlobTxResp(const error_type &error,
                                  handle_type& requestId) = 0;

    virtual void updateBlobResp(const error_type &error,
                                handle_type& requestId) = 0;
    virtual void updateBlobOnceResp(const error_type &error,
                                    handle_type& requestId) = 0;
    virtual void updateMetadataResp(const error_type &error,
                                    handle_type& requestId) = 0;
    virtual void renameBlobResp(const error_type &error,
                                handle_type& requestId,
                                shared_descriptor_type& blobDesc) = 0;
    virtual void deleteBlobResp(const error_type &error,
                                handle_type& requestId) = 0;

    virtual void statBlobResp(const error_type &error,
                              handle_type& requestId,
                              shared_descriptor_type& blobDesc) = 0;
    virtual void volumeStatusResp(const error_type &error,
                                  handle_type& requestId,
                                  shared_status_type& volumeStatus) = 0;
    virtual void volumeContentsResp(
        const error_type &error,
        handle_type& requestId,
        shared_descriptor_vec_type& volContents,
        shared_string_vec_type& skippedPrefixes) = 0;

    virtual void setVolumeMetadataResp(const error_type &error,
                                       handle_type& requestId) = 0;

    virtual void getVolumeMetadataResp(const error_type &error,
                                       handle_type& requestId,
                                       shared_meta_type& metadata) = 0;

    virtual void getBlobResp(const error_type &error,
                             handle_type& requestId,
                             shared_buffer_array_type const& buf,
                             size_type& length) = 0;
    virtual void getBlobWithMetaResp(const error_type &error,
                                     handle_type& requestId,
                                     shared_buffer_array_type const& buf,
                                     size_type& length,
                                     shared_descriptor_type& blobDesc) = 0;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMASYNCRESPONSEAPI_H_
