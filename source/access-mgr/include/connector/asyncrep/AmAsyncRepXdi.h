/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMASYNCREPXDI_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMASYNCREPXDI_H_

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include "AmAsyncResponseApi.h"
#include "AmAsyncDataApi.h"
#include "fdsp/AsyncXdiServiceResponse.h"

#include "connector/xdi/AmAsyncXdi.h"

namespace fds
{

// Currently, no special treatment of responses for replicated operations. They work just like any AmAsyncXdiResponse.

// This structure implements the Thrift XDI Service Request interface for
// asynchronously replicated transactions. Requests are uniquely identified
// by apis::RequestId. XDI Clients using this interface also
// require a handshake method implemented here.
//
// We don't want to pollute the other XDI clients with things they don't want to use or
// need so I've implemented handshake here and forwarded the rest of the
// API calls. The forwarding will likely be optimized out.
struct AmAsyncRepXdiRequest
    : public fds::apis::AsyncXdiServiceRequestIf,
      public AmAsyncDataApi<boost::shared_ptr<apis::RequestId>>
{
    using api_type = AmAsyncDataApi<boost::shared_ptr<apis::RequestId>>;

    explicit AmAsyncRepXdiRequest(std::shared_ptr<AmProcessor> processor, boost::shared_ptr<AmAsyncResponseApi<api_type::handle_type>> response_api):
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
    { LOGIO << " async rep op [" << op << "] handle [" << handle << "] blob [" << *blobName << "]"; }

    static
    void logio(char const* op,
          api_type::handle_type& handle,
          api_type::shared_string_type& blobName,
          api_type::shared_int_type& length,
          api_type::shared_offset_type& offset)
    {
        LOGIO << " async rep op [" << op
              << "] handle [" << handle
              << "] blob [" << *blobName
              << "] offset {" << std::hex << offset
              << "} length {" << std::dec << *length << "}";
    }

    template<typename AmRequestGenerator>
    void enqueue_async_rep_req(AmRequestGenerator reqGenFunc)
    {
        AmRequest *req = reqGenFunc();
        if (req != nullptr) {
            req->replicated = true;
            amProcessor->enqueueRequest(req);
        }
    }

    // These just mark the operation as replicated and forward to the generic template implementation in
    // AmAsyncDataApi_impl.h
    void abortBlobTx(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName, api_type::shared_string_type& blobName, api_type::shared_tx_ctx_type& txDesc)  // NOLINT
    { enqueue_async_rep_req([&] ()->AmRequest* {return api_type::abortBlobTx(requestId, domainName, volumeName, blobName, txDesc);}); }  // NOLINT
    void attachVolume(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName, api_type::shared_vol_mode_type& mode)  // NOLINT
    { enqueue_async_rep_req([&] ()->AmRequest* {return api_type::attachVolume(requestId, domainName, volumeName, mode);}); }
    void commitBlobTx(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName, api_type::shared_string_type& blobName, api_type::shared_tx_ctx_type& txDesc)  // NOLINT
    { enqueue_async_rep_req([&] ()->AmRequest* {return api_type::commitBlobTx(requestId, domainName, volumeName, blobName, txDesc);}); }
    void deleteBlob(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName, api_type::shared_string_type& blobName, api_type::shared_tx_ctx_type& txDesc)  // NOLINT
    { logio(__func__, requestId, blobName); enqueue_async_rep_req([&] ()->AmRequest* {return api_type::deleteBlob(requestId, domainName, volumeName, blobName, txDesc);}); }
    void getBlob(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName, api_type::shared_string_type& blobName, api_type::shared_int_type& length, api_type::shared_offset_type& offset)  // NOLINT
    { logio(__func__, requestId, blobName, length, offset); enqueue_async_rep_req([&] ()->AmRequest* {return api_type::getBlob(requestId, domainName, volumeName, blobName, length, offset);}); }  // NOLINT
    void getBlobWithMeta(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName, api_type::shared_string_type& blobName, api_type::shared_int_type& length, api_type::shared_offset_type& offset)  // NOLINT
    { logio(__func__, requestId, blobName, length, offset); enqueue_async_rep_req([&] ()->AmRequest* {return api_type::getBlobWithMeta(requestId, domainName, volumeName, blobName, length, offset);}); }  // NOLINT
    void renameBlob(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName, api_type::shared_string_type& sourceBlobName, api_type::shared_string_type& destinationBlobName)  // NOLINT
    { enqueue_async_rep_req([&] ()->AmRequest* {return api_type::renameBlob(requestId, domainName, volumeName, sourceBlobName, destinationBlobName);}); }  // NOLINT
    void startBlobTx(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName, api_type::shared_string_type& blobName, api_type::shared_int_type& blobMode)  // NOLINT
    { enqueue_async_rep_req([&] ()->AmRequest* {return api_type::startBlobTx(requestId, domainName, volumeName, blobName, blobMode);}); }
    void statBlob(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName, api_type::shared_string_type& blobName)  // NOLINT
    { enqueue_async_rep_req([&] ()->AmRequest* {return api_type::statBlob(requestId, domainName, volumeName, blobName);}); }
    void updateBlob(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName, api_type::shared_string_type& blobName, api_type::shared_tx_ctx_type& txDesc, api_type::shared_string_type& bytes, api_type::shared_int_type& length, api_type::shared_offset_type& objectOffset)  // NOLINT
    { logio(__func__, requestId, blobName, length, objectOffset); enqueue_async_rep_req([&] ()->AmRequest* {return api_type::updateBlob(requestId, domainName, volumeName, blobName, txDesc, bytes, length, objectOffset);}); }   // NOLINT
    void updateBlobOnce(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName, api_type::shared_string_type& blobName, api_type::shared_int_type& blobMode, api_type::shared_string_type& bytes, api_type::shared_int_type& length, api_type::shared_offset_type& objectOffset, api_type::shared_meta_type& metadata)  // NOLINT
    { logio(__func__, requestId, blobName, length, objectOffset); enqueue_async_rep_req([&] ()->AmRequest* {return api_type::updateBlobOnce(requestId, domainName, volumeName, blobName, blobMode, bytes, length, objectOffset, metadata);}); }   // NOLINT
    void updateMetadata(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName, api_type::shared_string_type& blobName, api_type::shared_tx_ctx_type& txDesc, api_type::shared_meta_type& metadata)  // NOLINT
    { enqueue_async_rep_req([&] ()->AmRequest* {return api_type::updateMetadata(requestId, domainName, volumeName, blobName, txDesc, metadata);}); }
    void volumeContents(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName, api_type::shared_int_type& count, api_type::shared_size_type& offset, api_type::shared_string_type& pattern, boost::shared_ptr<fpi::PatternSemantics>& patternSems, boost::shared_ptr<fpi::BlobListOrder>& orderBy, api_type::shared_bool_type& descending, api_type::shared_string_type& delimiter)  // NOLINT
    { enqueue_async_rep_req([&] ()->AmRequest* {return api_type::volumeContents(requestId, domainName, volumeName, count, offset, pattern, patternSems, orderBy, descending, delimiter);}); }
    void volumeStatus(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName)  // NOLINT
    { enqueue_async_rep_req([&] ()->AmRequest* {return api_type::volumeStatus(requestId, domainName, volumeName);}); }
    void setVolumeMetadata(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName, api_type::shared_meta_type& metadata)  // NOLINT
    { enqueue_async_rep_req([&] ()->AmRequest* {return api_type::setVolumeMetadata(requestId, domainName, volumeName, metadata);}); }
    void getVolumeMetadata(api_type::handle_type& requestId, api_type::shared_string_type& domainName, api_type::shared_string_type& volumeName)  // NOLINT
    { enqueue_async_rep_req([&] ()->AmRequest* {return api_type::getVolumeMetadata(requestId, domainName, volumeName);}); }

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
    void renameBlob(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const std::string& sourceBlobName, const std::string& destinationBlobName)  // NOLINT
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
    void volumeContents(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const int32_t count, const int64_t offset, const std::string& pattern, const fpi::PatternSemantics paternSems, const fpi::BlobListOrder orderBy, const bool descending, const std::string& delimiter)  // NOLINT
    { you_should_not_be_here(); }
    void volumeStatus(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName)  // NOLINT
    { you_should_not_be_here(); }
    void setVolumeMetadata(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName, const std::map<std::string, std::string>& metadata)  // NOLINT
    { you_should_not_be_here(); }
    void getVolumeMetadata(const apis::RequestId& requestId, const std::string& domainName, const std::string& volumeName)  // NOLINT
    { you_should_not_be_here(); }
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMASYNCREPXDI_H_
