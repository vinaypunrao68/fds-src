/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMASYNCXDI_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMASYNCXDI_H_

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "AmAsyncResponseApi.h"
#include "AmAsyncDataApi.h"
#include "fdsp/AsyncXdiServiceResponse.h"

namespace fds
{

class AmAsyncXdiResponse : public AmAsyncResponseApi<boost::shared_ptr<apis::RequestId>> {
 public:
    using client_type = apis::AsyncXdiServiceResponseClient;
    using client_ptr = std::shared_ptr<client_type>;
    using client_map = std::unordered_map<std::string, client_ptr>;

 private:
    using api_type = AmAsyncResponseApi<boost::shared_ptr<apis::RequestId>>;

    // We use a std rw lock here and vector or client pointers because
    // this lookup only happens once when the handshake is performed
    static std::mutex map_lock;
    static client_map clients;
    static constexpr size_t max_response_retries {1};


    /// Thrift client to response to XDI
    std::mutex client_lock;
    client_ptr asyncRespClient;
    std::string serverIp;
    fds_uint32_t serverPort;

    void initiateClientConnect();

    template<typename ... Args>
    void xdiClientCall(void (client_type::*func)(Args...), Args&&... args) {
        using transport_exception = apache::thrift::transport::TTransportException;
        std::lock_guard<std::mutex> g(client_lock);
        for (auto i = max_response_retries; i >= 0; --i) {
        try {
            if (!asyncRespClient) {
                initiateClientConnect();
            }
            // Invoke the thrift method on our client
            return ((asyncRespClient.get())->*(func))(std::forward<Args>(args)...);
        } catch(const transport_exception& e) {
            // Reset the pointer and re-try (if we have any left)
            try {
            initiateClientConnect();
            } catch (const transport_exception& e) {
                // ugh, Xdi probably died or we've become partitioned
                // assume we'll never return this response
                break;
            }
        }
        }
        LOGERROR << "Unable to respond to XDI: "
                 << "tcp://" << serverIp << ":" << serverPort;
    }

    boost::shared_ptr<fpi::ErrorCode> mappedErrorCode(Error const& error) const;

  public:
    explicit AmAsyncXdiResponse(std::string const& server_ip);
    virtual ~AmAsyncXdiResponse();
    typedef boost::shared_ptr<AmAsyncXdiResponse> shared_ptr;

    // This only belongs to the Thrift interface, not the AmAsyncData interface
    // to setup the response port.
    void handshakeComplete(api_type::handle_type& requestId,
                           boost::shared_ptr<int32_t>& port);

    void attachVolumeResp(const api_type::error_type &error,
                          api_type::handle_type& requestId,
                          api_type::shared_vol_descriptor_type& volDesc,
                          api_type::shared_vol_mode_type& mode) override;

    void detachVolumeResp(const api_type::error_type &error,
                          api_type::handle_type& requestId) override;

    void startBlobTxResp(const api_type::error_type &error,
                         api_type::handle_type& requestId,
                         boost::shared_ptr<apis::TxDescriptor>& txDesc) override;

    void abortBlobTxResp(const api_type::error_type &error,
                         api_type::handle_type& requestId) override;

    void commitBlobTxResp(const api_type::error_type &error,
                          api_type::handle_type& requestId) override;

    void updateBlobResp(const api_type::error_type &error,
                        api_type::handle_type& requestId) override;

    void updateBlobOnceResp(const api_type::error_type &error,
                            api_type::handle_type& requestId) override;

    void updateMetadataResp(const api_type::error_type &error,
                            api_type::handle_type& requestId) override;

    void deleteBlobResp(const api_type::error_type &error,
                        api_type::handle_type& requestId) override;

    void statBlobResp(const api_type::error_type &error,
                      api_type::handle_type& requestId,
                      api_type::shared_descriptor_type& blobDesc) override;

    void volumeStatusResp(const api_type::error_type &error,
                          api_type::handle_type& requestId,
                          api_type::shared_status_type& volumeStatus) override;

    void volumeContentsResp(const api_type::error_type &error,
                            api_type::handle_type& requestId,
                            api_type::shared_descriptor_vec_type& volContents) override;

    void setVolumeMetadataResp(const api_type::error_type &error,
                               api_type::handle_type& requestId) override;

    void getVolumeMetadataResp(const api_type::error_type &error,
                               api_type::handle_type& requestId,
                               api_type::shared_meta_type& metadata) override;

    void getBlobResp(const api_type::error_type &error,
                     api_type::handle_type& requestId,
                     const api_type::shared_buffer_array_type& bufs,
                     api_type::size_type& length) override;

    void getBlobWithMetaResp(const api_type::error_type &error,
                             api_type::handle_type& requestId,
                             const api_type::shared_buffer_array_type& bufs,
                             api_type::size_type& length,
                             api_type::shared_descriptor_type& blobDesc) override;
};

// This structure provides one feature and is to implement the thrift interface
// which uses apis::RequestId as a handle and has a handshake method. We don't
// want to pollute the other connectors with things they don't want to use or
// need so I'm implemented handshake here and forwarded the rest of the
// requests which will probably be optimized out.
struct AmAsyncXdiRequest
    : public fds::apis::AsyncXdiServiceRequestIf,
      public AmAsyncDataApi<boost::shared_ptr<apis::RequestId>>
{
    using api_type = AmAsyncDataApi<boost::shared_ptr<apis::RequestId>>;

    explicit AmAsyncXdiRequest(std::shared_ptr<AmProcessor> processor, boost::shared_ptr<AmAsyncResponseApi<api_type::handle_type>> response_api):
        api_type(processor, response_api)
    {}

    // This is only a Thrift interface, not a generic AmAsyncData one just to
    // setup the response port.
    void handshakeStart(api_type::handle_type& requestId, boost::shared_ptr<int32_t>& portNumber)  // NOLINT
    {
        auto api = boost::dynamic_pointer_cast<AmAsyncXdiResponse>(responseApi);
        if (api)
            api->handshakeComplete(requestId, portNumber);
    }

    static
    void logio(char const* op,
          api_type::handle_type& handle,
          api_type::shared_string_type& blobName)
    { LOGIO << " op [" << op << "] handle [" << handle << "] blob [" << *blobName << "]"; }

    static
    void logio(char const* op,
          api_type::handle_type& handle,
          api_type::shared_string_type& blobName,
          api_type::shared_int_type& length,
          api_type::shared_offset_type& offset)
    {
        LOGIO << " op [" << op
              << "] handle [" << handle
              << "] blob [" << *blobName
              << "] offset {" << std::hex << offset
              << "} length {" << std::dec << *length << "}";
    }

    // These just forward to the generic template implementation in
    // AmAsyncDataApi.cxx
    void abortBlobTx(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName, api_type::shared_string_type& blobName, api_type::shared_tx_ctx_type& txDesc)  // NOLINT
    { api_type::abortBlobTx(requestId, domainName, volumeName, blobName, txDesc); }  // NOLINT
    void attachVolume(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName, api_type::shared_vol_mode_type& mode)  // NOLINT
    { api_type::attachVolume(requestId, domainName, volumeName, mode); }
    void commitBlobTx(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName, api_type::shared_string_type& blobName, api_type::shared_tx_ctx_type& txDesc)  // NOLINT
    { api_type::commitBlobTx(requestId, domainName, volumeName, blobName, txDesc); }
    void deleteBlob(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName, api_type::shared_string_type& blobName, api_type::shared_tx_ctx_type& txDesc)  // NOLINT
    { logio(__func__, requestId, blobName); api_type::deleteBlob(requestId, domainName, volumeName, blobName, txDesc); }
    void getBlob(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName, api_type::shared_string_type& blobName, api_type::shared_int_type& length, api_type::shared_offset_type& offset)  // NOLINT
    { logio(__func__, requestId, blobName, length, offset); api_type::getBlob(requestId, domainName, volumeName, blobName, length, offset); }  // NOLINT
    void getBlobWithMeta(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName, api_type::shared_string_type& blobName, api_type::shared_int_type& length, api_type::shared_offset_type& offset)  // NOLINT
    { logio(__func__, requestId, blobName, length, offset); api_type::getBlobWithMeta(requestId, domainName, volumeName, blobName, length, offset); }  // NOLINT
    void startBlobTx(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName, api_type::shared_string_type& blobName, api_type::shared_int_type& blobMode)  // NOLINT
    { api_type::startBlobTx(requestId, domainName, volumeName, blobName, blobMode); }
    void statBlob(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName, api_type::shared_string_type& blobName)  // NOLINT
    { api_type::statBlob(requestId, domainName, volumeName, blobName); }
    void updateBlob(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName, api_type::shared_string_type& blobName, api_type::shared_tx_ctx_type& txDesc, api_type::shared_string_type& bytes, api_type::shared_int_type& length, api_type::shared_offset_type& objectOffset)  // NOLINT
    { logio(__func__, requestId, blobName, length, objectOffset); api_type::updateBlob(requestId, domainName, volumeName, blobName, txDesc, bytes, length, objectOffset); }   // NOLINT
    void updateBlobOnce(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName, api_type::shared_string_type& blobName, api_type::shared_int_type& blobMode, api_type::shared_string_type& bytes, api_type::shared_int_type& length, api_type::shared_offset_type& objectOffset, api_type::shared_meta_type& metadata)  // NOLINT
    { logio(__func__, requestId, blobName, length, objectOffset); api_type::updateBlobOnce(requestId, domainName, volumeName, blobName, blobMode, bytes, length, objectOffset, metadata); }   // NOLINT
    void updateMetadata(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName, api_type::shared_string_type& blobName, api_type::shared_tx_ctx_type& txDesc, api_type::shared_meta_type& metadata)  // NOLINT
    { api_type::updateMetadata(requestId, domainName, volumeName, blobName, txDesc, metadata); }
    void volumeContents(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName, api_type::shared_int_type& count, api_type::shared_size_type& offset, api_type::shared_string_type& pattern, boost::shared_ptr<fpi::BlobListOrder>& orderBy, api_type::shared_bool_type& descending)  // NOLINT
    { api_type::volumeContents(requestId, domainName, volumeName, count, offset, pattern, orderBy, descending); }
    void volumeStatus(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName)  // NOLINT
    { api_type::volumeStatus(requestId, domainName, volumeName); }
    void setVolumeMetadata(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName, api_type::shared_meta_type& metadata)  // NOLINT
    { api_type::setVolumeMetadata(requestId, domainName, volumeName, metadata); }
    void getVolumeMetadata(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName)  // NOLINT
    { api_type::getVolumeMetadata(requestId, domainName, volumeName); }

    // TODO(bszmyd): Tue 13 Jan 2015 04:00:24 PM PST
    // Delete these when we can. These are the synchronous forwarding.
    void you_should_not_be_here()
    { fds_panic("You shouldn't be here."); }
    void abortBlobTx(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const std::string& blobName, const apis::TxDescriptor& txDesc)  // NOLINT
    { you_should_not_be_here(); }
    void attachVolume(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const fpi::VolumeAccessMode& mode)  // NOLINT
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
    void updateBlob(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const std::string& blobName, const apis::TxDescriptor& txDesc, const std::string& bytes, const int32_t length, const apis::ObjectOffset& objectOffset)  // NOLINT
    { you_should_not_be_here(); }
    void updateBlobOnce(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const std::string& blobName, const int32_t blobMode, const std::string& bytes, const int32_t length, const apis::ObjectOffset& objectOffset, const std::map<std::string, std::string> & metadata)  // NOLINT
    { you_should_not_be_here(); }
    void volumeContents(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const int32_t count, const int64_t offset, const std::string& pattern, const fpi::BlobListOrder orderBy, const bool descending)  // NOLINT
    { you_should_not_be_here(); }
    void volumeStatus(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName)  // NOLINT
    { you_should_not_be_here(); }
    void setVolumeMetadata(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const std::map<std::string, std::string>& metadata)  // NOLINT
    { you_should_not_be_here(); }
    void getVolumeMetadata(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName)  // NOLINT
    { you_should_not_be_here(); }
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMASYNCXDI_H_
