/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMASYNCXDI_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMASYNCXDI_H_

#include <map>
#include <string>
#include <unordered_map>

#include "AmAsyncResponseApi.h"
#include "AmAsyncDataApi.h"
#include "apis/AsyncAmServiceResponse.h"
#include "concurrency/RwLock.h"

namespace fds
{

class AmAsyncXdiResponse : public AmAsyncResponseApi<boost::shared_ptr<apis::RequestId>> {
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

    void handshakeComplete(boost::shared_ptr<apis::RequestId>& requestId,
                           boost::shared_ptr<int32_t>& port);

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

struct AmAsyncXdiRequest
    : public fds::apis::AsyncAmServiceRequestIf,
      public AmAsyncDataApi<boost::shared_ptr<apis::RequestId>>  {
    typedef AmAsyncDataApi<boost::shared_ptr<apis::RequestId>> fds_api_type;
    explicit AmAsyncXdiRequest(boost::shared_ptr<AmAsyncResponseApi<boost::shared_ptr<apis::RequestId>>> response_api):
        fds_api_type(response_api)
    {}

    void abortBlobTx(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& volumeName, boost::shared_ptr<std::string>& blobName, boost::shared_ptr<apis::TxDescriptor>& txDesc)  // NOLINT
    { fds_api_type::abortBlobTx(requestId, domainName, volumeName, blobName, txDesc); }
    void attachVolume(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& volumeName)  // NOLINT
    { fds_api_type::attachVolume(requestId, domainName, volumeName); }
    void commitBlobTx(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& volumeName, boost::shared_ptr<std::string>& blobName, boost::shared_ptr<apis::TxDescriptor>& txDesc)  // NOLINT
    { fds_api_type::commitBlobTx(requestId, domainName, volumeName, blobName, txDesc); }
    void deleteBlob(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& volumeName, boost::shared_ptr<std::string>& blobName, boost::shared_ptr<apis::TxDescriptor>& txDesc)  // NOLINT
    { fds_api_type::deleteBlob(requestId, domainName, volumeName, blobName, txDesc); }
    void getBlob(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& volumeName, boost::shared_ptr<std::string>& blobName, boost::shared_ptr<int32_t>& length, boost::shared_ptr<apis::ObjectOffset>& offset)  // NOLINT
    { fds_api_type::getBlob(requestId, domainName, volumeName, blobName, length, offset); }
    void getBlobWithMeta(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& volumeName, boost::shared_ptr<std::string>& blobName, boost::shared_ptr<int32_t>& length, boost::shared_ptr<apis::ObjectOffset>& offset)  // NOLINT
    { fds_api_type::getBlobWithMeta(requestId, domainName, volumeName, blobName, length, offset); }
    void handshakeStart(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<int32_t>& portNumber)  // NOLINT
    {
        auto api = boost::dynamic_pointer_cast<AmAsyncXdiResponse>(responseApi);
        if (api)
            api->handshakeComplete(requestId, portNumber);
    }
    void startBlobTx(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& volumeName, boost::shared_ptr<std::string>& blobName, boost::shared_ptr<int32_t>& blobMode)  // NOLINT
    { fds_api_type::startBlobTx(requestId, domainName, volumeName, blobName, blobMode); }
    void statBlob(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& volumeName, boost::shared_ptr<std::string>& blobName)  // NOLINT
    { fds_api_type::statBlob(requestId, domainName, volumeName, blobName); }
    void updateBlob(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& volumeName, boost::shared_ptr<std::string>& blobName, boost::shared_ptr<apis::TxDescriptor>& txDesc, boost::shared_ptr<std::string>& bytes, boost::shared_ptr<int32_t>& length, boost::shared_ptr<apis::ObjectOffset>& objectOffset, boost::shared_ptr<bool>& isLast)  // NOLINT
    { fds_api_type::updateBlob(requestId, domainName, volumeName, blobName, txDesc, bytes, length, objectOffset, isLast); }   // NOLINT
    void updateBlobOnce(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& volumeName, boost::shared_ptr<std::string>& blobName, boost::shared_ptr<int32_t>& blobMode, boost::shared_ptr<std::string>& bytes, boost::shared_ptr<int32_t>& length, boost::shared_ptr<apis::ObjectOffset>& objectOffset, boost::shared_ptr<std::map<std::string, std::string> >& metadata)  // NOLINT
    { fds_api_type::updateBlobOnce(requestId, domainName, volumeName, blobName, blobMode, bytes, length, objectOffset, metadata); }   // NOLINT
    void updateMetadata(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& volumeName, boost::shared_ptr<std::string>& blobName, boost::shared_ptr<apis::TxDescriptor>& txDesc, boost::shared_ptr<std::map<std::string, std::string> >& metadata)  // NOLINT
    { fds_api_type::updateMetadata(requestId, domainName, volumeName, blobName, txDesc, metadata); }
    void volumeContents(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& volumeName, boost::shared_ptr<int32_t>& count, boost::shared_ptr<int64_t>& offset)  // NOLINT
    { fds_api_type::volumeContents(requestId, domainName, volumeName, count, offset); }
    void volumeStatus(boost::shared_ptr<apis::RequestId>& requestId, boost::shared_ptr<std::string>& domainName, boost::shared_ptr<std::string>& volumeName)  // NOLINT
    { fds_api_type::volumeStatus(requestId, domainName, volumeName); }

    void you_should_not_be_here()
    { fds_panic("You shouldn't be here."); }
    void abortBlobTx(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const std::string& blobName, const apis::TxDescriptor& txDesc)  // NOLINT
    { you_should_not_be_here(); }
    void attachVolume(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName)  // NOLINT
    { you_should_not_be_here(); }
    void commitBlobTx(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const std::string& blobName, const apis::TxDescriptor& txDesc)  // NOLINT
    { you_should_not_be_here(); }
    void deleteBlob(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const std::string& blobName, const apis::TxDescriptor& txDesc)  // NOLINT
    { you_should_not_be_here(); }
    void getBlob(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const std::string& blobName, const int32_t length, const apis::ObjectOffset& offset)  // NOLINT
    { you_should_not_be_here(); }
    void getBlobWithMeta(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const std::string& blobName, const int32_t length, const apis::ObjectOffset& offset)  // NOLINT
    { you_should_not_be_here(); }
    void handshakeStart(const apis::RequestId& requestId, const int32_t portNumber)
    { you_should_not_be_here(); }
    void startBlobTx(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const std::string& blobName, const int32_t blobMode)  // NOLINT
    { you_should_not_be_here(); }
    void statBlob(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const std::string& blobName)  // NOLINT
    { you_should_not_be_here(); }
    void updateMetadata(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const std::string& blobName, const apis::TxDescriptor& txDesc, const std::map<std::string, std::string> & metadata)  // NOLINT
    { you_should_not_be_here(); }
    void updateBlob(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const std::string& blobName, const apis::TxDescriptor& txDesc, const std::string& bytes, const int32_t length, const apis::ObjectOffset& objectOffset, const bool isLast)  // NOLINT
    { you_should_not_be_here(); }
    void updateBlobOnce(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const std::string& blobName, const int32_t blobMode, const std::string& bytes, const int32_t length, const apis::ObjectOffset& objectOffset, const std::map<std::string, std::string> & metadata)  // NOLINT
    { you_should_not_be_here(); }
    void volumeContents(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const int32_t count, const int64_t offset)  // NOLINT
    { you_should_not_be_here(); }
    void volumeStatus(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName)  // NOLINT
    { you_should_not_be_here(); }
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMASYNCXDI_H_
