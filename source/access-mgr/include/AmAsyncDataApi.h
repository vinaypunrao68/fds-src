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
template<typename H>
class AmAsyncDataApi {
    typedef H handle_type;
    typedef AmAsyncResponseApi<handle_type> response_api_type;
    typedef typename response_api_type::shared_ptr response_ptr;

  protected:
    /// Response client to use in response handler
    response_ptr responseApi;

  public:
    explicit AmAsyncDataApi(response_ptr response_api);
    explicit AmAsyncDataApi(response_api_type* response_api);

    ~AmAsyncDataApi() {}
    typedef std::shared_ptr<AmAsyncDataApi> shared_ptr;

    void attachVolume(boost::shared_ptr<handle_type>& requestId,
                      boost::shared_ptr<std::string>& domainName,
                      boost::shared_ptr<std::string>& volumeName);

    void volumeStatus(boost::shared_ptr<handle_type>& requestId,
                      boost::shared_ptr<std::string>& domainName,
                      boost::shared_ptr<std::string>& volumeName);

    void volumeContents(boost::shared_ptr<handle_type>& requestId,
                        boost::shared_ptr<std::string>& domainName,
                        boost::shared_ptr<std::string>& volumeName,
                        boost::shared_ptr<int32_t>& count,
                        boost::shared_ptr<int64_t>& offset);

    void statBlob(boost::shared_ptr<handle_type>& requestId,
                  boost::shared_ptr<std::string>& domainName,
                  boost::shared_ptr<std::string>& volumeName,
                  boost::shared_ptr<std::string>& blobName);

    void startBlobTx(boost::shared_ptr<handle_type>& requestId,
                     boost::shared_ptr<std::string>& domainName,
                     boost::shared_ptr<std::string>& volumeName,
                     boost::shared_ptr<std::string>& blobName,
                     boost::shared_ptr<fds_int32_t>& blobMode);

    void commitBlobTx(boost::shared_ptr<handle_type>& requestId,
                      boost::shared_ptr<std::string>& domainName,
                      boost::shared_ptr<std::string>& volumeName,
                      boost::shared_ptr<std::string>& blobName,
                      boost::shared_ptr<apis::TxDescriptor>& txDesc);

    void abortBlobTx(boost::shared_ptr<handle_type>& requestId,
                     boost::shared_ptr<std::string>& domainName,
                     boost::shared_ptr<std::string>& volumeName,
                     boost::shared_ptr<std::string>& blobName,
                     boost::shared_ptr<apis::TxDescriptor>& txDesc);

    void getBlob(boost::shared_ptr<handle_type>& requestId,
                 boost::shared_ptr<std::string>& domainName,
                 boost::shared_ptr<std::string>& volumeName,
                 boost::shared_ptr<std::string>& blobName,
                 boost::shared_ptr<int32_t>& length,
                 boost::shared_ptr<apis::ObjectOffset>& objectOffset);

    void getBlobWithMeta(boost::shared_ptr<handle_type>& requestId,
                         boost::shared_ptr<std::string>& domainName,
                         boost::shared_ptr<std::string>& volumeName,
                         boost::shared_ptr<std::string>& blobName,
                         boost::shared_ptr<int32_t>& length,
                         boost::shared_ptr<apis::ObjectOffset>& objectOffset);

    void updateMetadata(boost::shared_ptr<handle_type>& requestId,
                        boost::shared_ptr<std::string>& domainName,
                        boost::shared_ptr<std::string>& volumeName,
                        boost::shared_ptr<std::string>& blobName,
                        boost::shared_ptr<apis::TxDescriptor>& txDesc,
                        boost::shared_ptr< std::map<std::string, std::string> >& metadata);

    void updateBlobOnce(boost::shared_ptr<handle_type>& requestId,
                        boost::shared_ptr<std::string>& domainName,
                        boost::shared_ptr<std::string>& volumeName,
                        boost::shared_ptr<std::string>& blobName,
                        boost::shared_ptr<fds_int32_t>& blobMode,
                        boost::shared_ptr<std::string>& bytes,
                        boost::shared_ptr<int32_t>& length,
                        boost::shared_ptr<apis::ObjectOffset>& objectOffset,
                        boost::shared_ptr< std::map<std::string, std::string> >& metadata);

    void updateBlob(boost::shared_ptr<handle_type>& requestId,
                    boost::shared_ptr<std::string>& domainName,
                    boost::shared_ptr<std::string>& volumeName,
                    boost::shared_ptr<std::string>& blobName,
                    boost::shared_ptr<apis::TxDescriptor>& txDesc,
                    boost::shared_ptr<std::string>& bytes,
                    boost::shared_ptr<int32_t>& length,
                    boost::shared_ptr<apis::ObjectOffset>& objectOffset,
                    boost::shared_ptr<bool>& isLast);

    void deleteBlob(boost::shared_ptr<handle_type>& requestId,
                    boost::shared_ptr<std::string>& domainName,
                    boost::shared_ptr<std::string>& volumeName,
                    boost::shared_ptr<std::string>& blobName,
                    boost::shared_ptr<apis::TxDescriptor>& txDesc);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMASYNCDATAAPI_H_
