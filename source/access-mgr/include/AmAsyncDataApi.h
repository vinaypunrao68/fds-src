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
 public:
    template <typename M> using sp = boost::shared_ptr<M>;

    typedef H handle_type;
    typedef sp<bool> shared_bool_type;
    typedef sp<int32_t> shared_int_type;
    typedef sp<int64_t> shared_size_type;
    typedef sp<std::string> shared_buffer_type;
    typedef sp<std::string> shared_string_type;
    typedef sp<apis::ObjectOffset> shared_offset_type;
    typedef sp<apis::TxDescriptor> shared_tx_ctx_type;
    typedef sp<std::map<std::string, std::string>> shared_meta_type;

 private:
    typedef AmAsyncResponseApi<handle_type> response_api_type;
    typedef typename boost::shared_ptr<response_api_type> response_ptr;

  protected:
    /// Response client to use in response handler
    response_ptr responseApi;

  public:
    explicit AmAsyncDataApi(response_ptr response_api);
    explicit AmAsyncDataApi(response_api_type* response_api);

    ~AmAsyncDataApi() {}

    void attachVolume(handle_type& requestId,
                      shared_string_type& domainName,
                      shared_string_type& volumeName);

    void volumeStatus(handle_type& requestId,
                      shared_string_type& domainName,
                      shared_string_type& volumeName);

    void volumeContents(handle_type& requestId,
                        shared_string_type& domainName,
                        shared_string_type& volumeName,
                        shared_int_type& count,
                        shared_size_type& offset,
                        shared_string_type& pattern,
                        boost::shared_ptr<fpi::BlobListOrder>& orderBy,
                        shared_bool_type& descending);

    void statBlob(handle_type& requestId,
                  shared_string_type& domainName,
                  shared_string_type& volumeName,
                  shared_string_type& blobName);

    void startBlobTx(handle_type& requestId,
                     shared_string_type& domainName,
                     shared_string_type& volumeName,
                     shared_string_type& blobName,
                     shared_int_type& blobMode);

    void commitBlobTx(handle_type& requestId,
                      shared_string_type& domainName,
                      shared_string_type& volumeName,
                      shared_string_type& blobName,
                      shared_tx_ctx_type& txDesc);

    void abortBlobTx(handle_type& requestId,
                     shared_string_type& domainName,
                     shared_string_type& volumeName,
                     shared_string_type& blobName,
                     shared_tx_ctx_type& txDesc);

    void getBlob(handle_type& requestId,
                 shared_string_type& domainName,
                 shared_string_type& volumeName,
                 shared_string_type& blobName,
                 shared_int_type& length,
                 shared_offset_type& objectOffset);

    void getBlobWithMeta(handle_type& requestId,
                         shared_string_type& domainName,
                         shared_string_type& volumeName,
                         shared_string_type& blobName,
                         shared_int_type& length,
                         shared_offset_type& objectOffset);

    void updateMetadata(handle_type& requestId,
                        shared_string_type& domainName,
                        shared_string_type& volumeName,
                        shared_string_type& blobName,
                        shared_tx_ctx_type& txDesc,
                        shared_meta_type& metadata);

    void updateBlobOnce(handle_type& requestId,
                        shared_string_type& domainName,
                        shared_string_type& volumeName,
                        shared_string_type& blobName,
                        shared_int_type& blobMode,
                        shared_buffer_type& bytes,
                        shared_int_type& length,
                        shared_offset_type& objectOffset,
                        shared_meta_type& metadata);

    void updateBlob(handle_type& requestId,
                    shared_string_type& domainName,
                    shared_string_type& volumeName,
                    shared_string_type& blobName,
                    shared_tx_ctx_type& txDesc,
                    shared_buffer_type& bytes,
                    shared_int_type& length,
                    shared_offset_type& objectOffset,
                    shared_bool_type& isLast);

    void deleteBlob(handle_type& requestId,
                    shared_string_type& domainName,
                    shared_string_type& volumeName,
                    shared_string_type& blobName,
                    shared_tx_ctx_type& txDesc);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMASYNCDATAAPI_H_
