/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMASYNCRESPONSEAPI_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMASYNCRESPONSEAPI_H_

#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include <fds_module.h>
#include "concurrency/RwLock.h"
#include <apis/AsyncAmServiceResponse.h>

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
    typedef boost::shared_ptr<AmAsyncResponseApi> shared_ptr;

    virtual void attachVolumeResp(const Error &error,
                                  boost::shared_ptr<handle_type>& requestId) = 0;

    virtual void startBlobTxResp(const Error &error,
                                 boost::shared_ptr<handle_type>& requestId,
                                 boost::shared_ptr<apis::TxDescriptor>& txDesc) = 0;
    virtual void abortBlobTxResp(const Error &error,
                                 boost::shared_ptr<handle_type>& requestId) = 0;
    virtual void commitBlobTxResp(const Error &error,
                                  boost::shared_ptr<handle_type>& requestId) = 0;

    virtual void updateBlobResp(const Error &error,
                                boost::shared_ptr<handle_type>& requestId) = 0;
    virtual void updateBlobOnceResp(const Error &error,
                                    boost::shared_ptr<handle_type>& requestId) = 0;
    virtual void updateMetadataResp(const Error &error,
                                    boost::shared_ptr<handle_type>& requestId) = 0;
    virtual void deleteBlobResp(const Error &error,
                                boost::shared_ptr<handle_type>& requestId) = 0;

    virtual void statBlobResp(const Error &error,
                              boost::shared_ptr<handle_type>& requestId,
                              boost::shared_ptr<apis::BlobDescriptor>& blobDesc) = 0;
    virtual void volumeStatusResp(const Error &error,
                                  boost::shared_ptr<handle_type>& requestId,
                                  boost::shared_ptr<apis::VolumeStatus>& volumeStatus) = 0;
    virtual void volumeContentsResp(
        const Error &error,
        boost::shared_ptr<handle_type>& requestId,
        boost::shared_ptr<std::vector<apis::BlobDescriptor>>& volContents) = 0;

    virtual void getBlobResp(const Error &error,
                             boost::shared_ptr<handle_type>& requestId,
                             boost::shared_ptr<std::string> buf,
                             fds_uint32_t& length) = 0;
    virtual void getBlobWithMetaResp(const Error &error,
                                     boost::shared_ptr<handle_type>& requestId,
                                     boost::shared_ptr<std::string> buf,
                                     fds_uint32_t& length,
                                     boost::shared_ptr<apis::BlobDescriptor>& blobDesc) = 0;
};

class AmAsyncXdiResponse : public AmAsyncResponseApi<apis::RequestId> {
 public:
    using client_ptr = std::shared_ptr<apis::AsyncAmServiceResponseClient>;
    using client_map = std::unordered_map<std::string, client_ptr>;

 private:
    // We use a std rw lock here and vector or client pointers because
    // this lookup only happens once when the handshake is performed
    static fds_rwlock client_lock;
    static client_map clients;

    /// Thrift client to response to XDI
    client_ptr asyncRespClient;
    std::string serverIp;
    fds_uint32_t serverPort;

    void initiateClientConnect();
    inline void checkClientConnect() {
        if (asyncRespClient == NULL && serverPort > 0) {
            initiateClientConnect();
        }
        fds_assert(asyncRespClient);
    }

  public:
    explicit AmAsyncXdiResponse(std::string const& server_ip);
    ~AmAsyncXdiResponse();
    typedef boost::shared_ptr<AmAsyncXdiResponse> shared_ptr;

    void handshakeComplete(boost::shared_ptr<handle_type>& requestId,
                           boost::shared_ptr<int32_t>& port);

    void attachVolumeResp(const Error &error,
                          boost::shared_ptr<handle_type>& requestId);

    void startBlobTxResp(const Error &error,
                         boost::shared_ptr<handle_type>& requestId,
                         boost::shared_ptr<apis::TxDescriptor>& txDesc);
    void abortBlobTxResp(const Error &error,
                         boost::shared_ptr<handle_type>& requestId);
    void commitBlobTxResp(const Error &error,
                          boost::shared_ptr<handle_type>& requestId);

    void updateBlobResp(const Error &error,
                        boost::shared_ptr<handle_type>& requestId);
    void updateBlobOnceResp(const Error &error,
                            boost::shared_ptr<handle_type>& requestId);
    void updateMetadataResp(const Error &error,
                            boost::shared_ptr<handle_type>& requestId);
    void deleteBlobResp(const Error &error,
                        boost::shared_ptr<handle_type>& requestId);

    void statBlobResp(const Error &error,
                      boost::shared_ptr<handle_type>& requestId,
                      boost::shared_ptr<apis::BlobDescriptor>& blobDesc);
    void volumeStatusResp(const Error &error,
                          boost::shared_ptr<handle_type>& requestId,
                          boost::shared_ptr<apis::VolumeStatus>& volumeStatus);
    void volumeContentsResp(
        const Error &error,
        boost::shared_ptr<handle_type>& requestId,
        boost::shared_ptr<std::vector<apis::BlobDescriptor>>& volContents);

    void getBlobResp(const Error &error,
                     boost::shared_ptr<handle_type>& requestId,
                     boost::shared_ptr<std::string> buf,
                     fds_uint32_t& length);
    void getBlobWithMetaResp(const Error &error,
                             boost::shared_ptr<handle_type>& requestId,
                             boost::shared_ptr<std::string> buf,
                             fds_uint32_t& length,
                             boost::shared_ptr<apis::BlobDescriptor>& blobDesc);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMASYNCRESPONSEAPI_H_
