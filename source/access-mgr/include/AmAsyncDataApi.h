/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMASYNCDATAAPI_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMASYNCDATAAPI_H_

#include <map>
#include <string>
#include <vector>
#include <fds_module.h>
#include <apis/AsyncAmServiceRequest.h>
#include <AmAsyncResponseApi.h>

namespace fds {

/**
 * AM's async data API that is exposed to XDI. This interface is the
 * basic data API that XDI and connectors are programmed to.
 */
class AmAsyncDataApi : public apis::AsyncAmServiceRequestIf {
  private:
    /// Uturn test all AM service APIs
    fds_bool_t testUturnAll;
    /// Uturn test start tx API
    fds_bool_t testUturnStartTx;
    /// Uturn test update blob API
    fds_bool_t testUturnUpdateBlob;
    /// Uturn test update metadata API
    fds_bool_t testUturnUpdateMeta;
    /// Uturn test commit tx API
    fds_bool_t testUturnCommitTx;
    /// Uturn test abort tx API
    fds_bool_t testUturnAbortTx;
    /// Uturn test stat blob API
    fds_bool_t testUturnStatBlob;
    /// Uturn test get blob API
    fds_bool_t testUturnGetBlob;

    /// Response client to use in response handler
    AmAsyncResponseApi::shared_ptr responseApi;

  public:
    AmAsyncDataApi();
    ~AmAsyncDataApi();
    typedef boost::shared_ptr<AmAsyncDataApi> shared_ptr;

    void setResponseApi(AmAsyncResponseApi::shared_ptr respApi);

    void volumeStatus(const apis::RequestId& requestId,
                      const std::string& domainName,
                      const std::string& volumeName);
    void volumeStatus(boost::shared_ptr<apis::RequestId>& requestId,
                      boost::shared_ptr<std::string>& domainName,
                      boost::shared_ptr<std::string>& volumeName);

    void statBlob(const apis::RequestId& requestId,
                  const std::string& domainName,
                  const std::string& volumeName,
                  const std::string& blobName);
    void statBlob(boost::shared_ptr<apis::RequestId>& requestId,
                  boost::shared_ptr<std::string>& domainName,
                  boost::shared_ptr<std::string>& volumeName,
                  boost::shared_ptr<std::string>& blobName);

    void startBlobTx(const apis::RequestId& requestId,
                     const std::string& domainName,
                     const std::string& volumeName,
                     const std::string& blobName,
                     const fds_int32_t blobMode);

    void startBlobTx(boost::shared_ptr<apis::RequestId>& requestId,
                     boost::shared_ptr<std::string>& domainName,
                     boost::shared_ptr<std::string>& volumeName,
                     boost::shared_ptr<std::string>& blobName,
                     boost::shared_ptr<fds_int32_t>& blobMode);

    void commitBlobTx(const apis::RequestId& requestId,
                      const std::string& domainName,
                      const std::string& volumeName,
                      const std::string& blobName,
                      const apis::TxDescriptor& txDesc);
    void commitBlobTx(boost::shared_ptr<apis::RequestId>& requestId,
                      boost::shared_ptr<std::string>& domainName,
                      boost::shared_ptr<std::string>& volumeName,
                      boost::shared_ptr<std::string>& blobName,
                      boost::shared_ptr<apis::TxDescriptor>& txDesc);

    void abortBlobTx(const apis::RequestId& requestId,
                     const std::string& domainName,
                     const std::string& volumeName,
                     const std::string& blobName,
                     const apis::TxDescriptor& txDesc);
    void abortBlobTx(boost::shared_ptr<apis::RequestId>& requestId,
                     boost::shared_ptr<std::string>& domainName,
                     boost::shared_ptr<std::string>& volumeName,
                     boost::shared_ptr<std::string>& blobName,
                     boost::shared_ptr<apis::TxDescriptor>& txDesc);

    void getBlob(const apis::RequestId& requestId,
                 const std::string& domainName,
                 const std::string& volumeName,
                 const std::string& blobName,
                 const int32_t length,
                 const apis::ObjectOffset& offset);
    void getBlob(boost::shared_ptr<apis::RequestId>& requestId,
                 boost::shared_ptr<std::string>& domainName,
                 boost::shared_ptr<std::string>& volumeName,
                 boost::shared_ptr<std::string>& blobName,
                 boost::shared_ptr<int32_t>& length,
                 boost::shared_ptr<apis::ObjectOffset>& objectOffset);

    void updateMetadata(const apis::RequestId& requestId,
                        const std::string& domainName,
                        const std::string& volumeName,
                        const std::string& blobName,
                        const apis::TxDescriptor& txDesc,
                        const std::map<std::string, std::string> & metadata);
    void updateMetadata(boost::shared_ptr<apis::RequestId>& requestId,
                        boost::shared_ptr<std::string>& domainName,
                        boost::shared_ptr<std::string>& volumeName,
                        boost::shared_ptr<std::string>& blobName,
                        boost::shared_ptr<apis::TxDescriptor>& txDesc,
                        boost::shared_ptr< std::map<std::string, std::string> >& metadata);

    void updateBlobOnce(const apis::RequestId& requestId,
                        const std::string& domainName,
                        const std::string& volumeName,
                        const std::string& blobName,
                        const fds_int32_t blobMode,
                        const std::string& bytes,
                        const int32_t length,
                        const apis::ObjectOffset& objectOffset,
                        const std::map<std::string, std::string>& metadata);
    void updateBlobOnce(boost::shared_ptr<apis::RequestId>& requestId,
                        boost::shared_ptr<std::string>& domainName,
                        boost::shared_ptr<std::string>& volumeName,
                        boost::shared_ptr<std::string>& blobName,
                        boost::shared_ptr<fds_int32_t>& blobMode,
                        boost::shared_ptr<std::string>& bytes,
                        boost::shared_ptr<int32_t>& length,
                        boost::shared_ptr<apis::ObjectOffset>& objectOffset,
                        boost::shared_ptr< std::map<std::string, std::string> >& metadata);

    void updateBlob(const apis::RequestId& requestId,
                    const std::string& domainName,
                    const std::string& volumeName,
                    const std::string& blobName,
                    const apis::TxDescriptor& txDesc,
                    const std::string& bytes,
                    const int32_t length,
                    const apis::ObjectOffset& objectOffset,
                    const bool isLast);
    void updateBlob(boost::shared_ptr<apis::RequestId>& requestId,
                    boost::shared_ptr<std::string>& domainName,
                    boost::shared_ptr<std::string>& volumeName,
                    boost::shared_ptr<std::string>& blobName,
                    boost::shared_ptr<apis::TxDescriptor>& txDesc,
                    boost::shared_ptr<std::string>& bytes,
                    boost::shared_ptr<int32_t>& length,
                    boost::shared_ptr<apis::ObjectOffset>& objectOffset,
                    boost::shared_ptr<bool>& isLast);

    void deleteBlob(const apis::RequestId& requestId,
                    const std::string& domainName,
                    const std::string& volumeName,
                    const std::string& blobName);
    void deleteBlob(boost::shared_ptr<apis::RequestId>& requestId,
                    boost::shared_ptr<std::string>& domainName,
                    boost::shared_ptr<std::string>& volumeName,
                    boost::shared_ptr<std::string>& blobName);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMASYNCDATAAPI_H_
