/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMASYNCRESPONSEAPI_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMASYNCRESPONSEAPI_H_

#include <map>
#include <string>
#include <vector>
#include <fds_module.h>
#include <apis/AsyncAmServiceResponse.h>

namespace fds {

/**
 * AM's data API that is exposed to XDI. This interface is the
 * basic data API that XDI and connectors are programmed to. A
 * pure virtual interface is exposed so that the implementation
 * of the responses can be overloaded (e.g., respond to XDI, respond
 * to unit test, etc...).
 */
class AmAsyncResponseApi {
  public:
    typedef boost::shared_ptr<AmAsyncResponseApi> shared_ptr;

    virtual void attachVolumeResp(const Error &error,
                                  boost::shared_ptr<apis::RequestId>& requestId) = 0;

    virtual void startBlobTxResp(const Error &error,
                                 boost::shared_ptr<apis::RequestId>& requestId,
                                 boost::shared_ptr<apis::TxDescriptor>& txDesc) = 0;
    virtual void abortBlobTxResp(const Error &error,
                                 boost::shared_ptr<apis::RequestId>& requestId) = 0;
    virtual void commitBlobTxResp(const Error &error,
                                  boost::shared_ptr<apis::RequestId>& requestId) = 0;

    virtual void updateBlobResp(const Error &error,
                                boost::shared_ptr<apis::RequestId>& requestId) = 0;
    virtual void updateBlobOnceResp(const Error &error,
                                    boost::shared_ptr<apis::RequestId>& requestId) = 0;
    virtual void updateMetadataResp(const Error &error,
                                    boost::shared_ptr<apis::RequestId>& requestId) = 0;
    virtual void deleteBlobResp(const Error &error,
                                boost::shared_ptr<apis::RequestId>& requestId) = 0;

    virtual void statBlobResp(const Error &error,
                              boost::shared_ptr<apis::RequestId>& requestId,
                              boost::shared_ptr<apis::BlobDescriptor>& blobDesc) = 0;
    virtual void volumeStatusResp(const Error &error,
                                  boost::shared_ptr<apis::RequestId>& requestId,
                                  boost::shared_ptr<apis::VolumeStatus>& volumeStatus) = 0;
    virtual void volumeContentsResp(
        const Error &error,
        boost::shared_ptr<apis::RequestId>& requestId,
        boost::shared_ptr<std::vector<apis::BlobDescriptor>>& volContents) = 0;

    virtual void getBlobResp(const Error &error,
                             boost::shared_ptr<apis::RequestId>& requestId,
                             boost::shared_ptr<std::string> buf,
                             fds_uint32_t& length) = 0;
    virtual void getBlobWithMetaResp(const Error &error,
                                     boost::shared_ptr<apis::RequestId>& requestId,
                                     boost::shared_ptr<std::string> buf,
                                     fds_uint32_t& length,
                                     boost::shared_ptr<apis::BlobDescriptor>& blobDesc) = 0;
};

class AmAsyncXdiResponse : public AmAsyncResponseApi {
  private:
    /// Thrift client to response to XDI
    boost::shared_ptr<apis::AsyncAmServiceResponseClient> asyncRespClient;
    std::string serverIp;
    fds_uint32_t serverPort;

    void initiateClientConnect();
    inline void checkClientConnect() {
        if (asyncRespClient == NULL) {
            initiateClientConnect();
        }
    }

  public:
    AmAsyncXdiResponse();
    ~AmAsyncXdiResponse();
    typedef boost::shared_ptr<AmAsyncXdiResponse> shared_ptr;

    void attachVolumeResp(const Error &error,
                          boost::shared_ptr<apis::RequestId>& requestId);

    void startBlobTxResp(const Error &error,
                         boost::shared_ptr<apis::RequestId>& requestId,
                         boost::shared_ptr<apis::TxDescriptor>& txDesc);
    void abortBlobTxResp(const Error &error,
                         boost::shared_ptr<apis::RequestId>& requestId);
    void commitBlobTxResp(const Error &error,
                          boost::shared_ptr<apis::RequestId>& requestId);

    void updateBlobResp(const Error &error,
                        boost::shared_ptr<apis::RequestId>& requestId);
    void updateBlobOnceResp(const Error &error,
                            boost::shared_ptr<apis::RequestId>& requestId);
    void updateMetadataResp(const Error &error,
                            boost::shared_ptr<apis::RequestId>& requestId);
    void deleteBlobResp(const Error &error,
                        boost::shared_ptr<apis::RequestId>& requestId);

    void statBlobResp(const Error &error,
                      boost::shared_ptr<apis::RequestId>& requestId,
                      boost::shared_ptr<apis::BlobDescriptor>& blobDesc);
    void volumeStatusResp(const Error &error,
                          boost::shared_ptr<apis::RequestId>& requestId,
                          boost::shared_ptr<apis::VolumeStatus>& volumeStatus);
    void volumeContentsResp(
        const Error &error,
        boost::shared_ptr<apis::RequestId>& requestId,
        boost::shared_ptr<std::vector<apis::BlobDescriptor>>& volContents);

    void getBlobResp(const Error &error,
                     boost::shared_ptr<apis::RequestId>& requestId,
                     boost::shared_ptr<std::string> buf,
                     fds_uint32_t& length);
    void getBlobWithMetaResp(const Error &error,
                             boost::shared_ptr<apis::RequestId>& requestId,
                             boost::shared_ptr<std::string> buf,
                             fds_uint32_t& length,
                             boost::shared_ptr<apis::BlobDescriptor>& blobDesc);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMASYNCRESPONSEAPI_H_
