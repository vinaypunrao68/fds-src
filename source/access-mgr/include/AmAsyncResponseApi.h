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
    typedef H handle_type;

    virtual void attachVolumeResp(const Error &error,
                                  handle_type& requestId) = 0;

    virtual void startBlobTxResp(const Error &error,
                                 handle_type& requestId,
                                 boost::shared_ptr<apis::TxDescriptor>& txDesc) = 0;
    virtual void abortBlobTxResp(const Error &error,
                                 handle_type& requestId) = 0;
    virtual void commitBlobTxResp(const Error &error,
                                  handle_type& requestId) = 0;

    virtual void updateBlobResp(const Error &error,
                                handle_type& requestId) = 0;
    virtual void updateBlobOnceResp(const Error &error,
                                    handle_type& requestId) = 0;
    virtual void updateMetadataResp(const Error &error,
                                    handle_type& requestId) = 0;
    virtual void deleteBlobResp(const Error &error,
                                handle_type& requestId) = 0;

    virtual void statBlobResp(const Error &error,
                              handle_type& requestId,
                              boost::shared_ptr<apis::BlobDescriptor>& blobDesc) = 0;
    virtual void volumeStatusResp(const Error &error,
                                  handle_type& requestId,
                                  boost::shared_ptr<apis::VolumeStatus>& volumeStatus) = 0;
    virtual void volumeContentsResp(
        const Error &error,
        handle_type& requestId,
        boost::shared_ptr<std::vector<apis::BlobDescriptor>>& volContents) = 0;

    virtual void getBlobResp(const Error &error,
                             handle_type& requestId,
                             boost::shared_ptr<std::string> buf,
                             fds_uint32_t& length) = 0;
    virtual void getBlobWithMetaResp(const Error &error,
                                     handle_type& requestId,
                                     boost::shared_ptr<std::string> buf,
                                     fds_uint32_t& length,
                                     boost::shared_ptr<apis::BlobDescriptor>& blobDesc) = 0;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMASYNCRESPONSEAPI_H_
