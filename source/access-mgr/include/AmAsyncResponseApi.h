/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMASYNCRESPONSEAPI_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMASYNCRESPONSEAPI_H_

#include <string>
#include <vector>
#include "apis/apis_types.h"
#include "fds_error.h"

namespace fds {

/**
 * AM's data API that is exposed to XDI. This interface is the
 * basic data API that XDI and connectors are programmed to. A
 * pure virtual interface is exposed so that the implementation
 * of the responses can be overloaded (e.g., respond to XDI, respond
 * to unit test, etc...).
 */
template<typename H>
class AmAsyncResponseApi {
  public:
    template <typename M> using sp = boost::shared_ptr<M>;
  
    typedef H handle_type;
    typedef Error error_type;
    typedef uint32_t size_type;
    typedef sp<std::string> shared_buffer_type;
    typedef sp<apis::BlobDescriptor> shared_descriptor_type;
    typedef sp<std::vector<apis::BlobDescriptor>> shared_descriptor_vec_type;
    typedef sp<apis::TxDescriptor> shared_tx_ctx_type;
    typedef sp<apis::VolumeStatus> shared_status_type;

    virtual void attachVolumeResp(const error_type &error,
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
        shared_descriptor_vec_type& volContents) = 0;

    virtual void getBlobResp(const error_type &error,
                             handle_type& requestId,
                             shared_buffer_type buf,
                             fds_uint32_t& length) = 0;
    virtual void getBlobWithMetaResp(const error_type &error,
                                     handle_type& requestId,
                                     shared_buffer_type buf,
                                     fds_uint32_t& length,
                                     shared_descriptor_type& blobDesc) = 0;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMASYNCRESPONSEAPI_H_
