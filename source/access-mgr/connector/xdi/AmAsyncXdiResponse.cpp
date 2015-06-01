/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <vector>
#include "AmAsyncResponseApi.h"
#include "connector/xdi/AmAsyncXdi.h"
#include <blob/BlobTypes.h>

#include <arpa/inet.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/server/TNonblockingServer.h>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>

namespace fds {

namespace xdi_att = apache::thrift::transport;
namespace xdi_atp = apache::thrift::protocol;

#define XDICLIENTCALL(func)                           \
    checkClientConnect();                             \
    try {                                             \
        SCOPEDREAD(client_lock);                      \
        if (asyncRespClient) {                        \
            asyncRespClient->func;                    \
        } else {                                      \
            LOGDEBUG << "No response channel, dropping response message!"; \
        }                                             \
    } catch(const xdi_att::TTransportException& e) {  \
        SCOPEDWRITE(client_lock);                     \
        asyncRespClient.reset();                      \
        LOGERROR << "Unable to respond to XDI: "      \
                 << "tcp://" << serverIp << ":" << serverPort \
                 << "; Dropping response and uninitializing the connection!"; \
    }

fds_rwlock AmAsyncXdiResponse::client_lock;
AmAsyncXdiResponse::client_map AmAsyncXdiResponse::clients;

AmAsyncXdiResponse::AmAsyncXdiResponse(std::string const& server_ip)
        : serverIp(server_ip),
          serverPort(9876) {
    // Set client to unitialized
    asyncRespClient.reset();
}

AmAsyncXdiResponse::~AmAsyncXdiResponse() {
    // See if we're the last connected to this client
    // if so we need to remove it from the client vector
    SCOPEDWRITE(client_lock);
    if (asyncRespClient.use_count() <= 2) {
        std::string tcp_nexus(serverIp + ":" + std::to_string(serverPort));
        auto client_it = clients.find(tcp_nexus);
        if (client_it != clients.end())
            clients.erase(client_it);
        asyncRespClient.reset();
    }
}

void
AmAsyncXdiResponse::initiateClientConnect() {
    std::string tcp_nexus(serverIp + ":" + std::to_string(serverPort));
    // First see if we're already connected to this client
    // if so, reuse response path
    SCOPEDWRITE(client_lock);
    auto client_it = clients.find(tcp_nexus);

    if (client_it != clients.end())
    {
        LOGTRACE << "Found response channel!";
        asyncRespClient = client_it->second;
    } else {
        LOGNORMAL << "Setting up response channel to: "
                  << "tcp://" << serverIp << ":" << serverPort;
        // Setup the async response client
        boost::shared_ptr<xdi_att::TTransport> respSock(
            boost::make_shared<xdi_att::TSocket>(
                serverIp, serverPort));
        boost::shared_ptr<xdi_att::TFramedTransport> respTrans(
            boost::make_shared<xdi_att::TFramedTransport>(respSock));
        boost::shared_ptr<xdi_atp::TProtocol> respProto(
            boost::make_shared<xdi_atp::TBinaryProtocol>(respTrans));
        asyncRespClient = std::make_shared<apis::AsyncXdiServiceResponseClient>(respProto);
        clients[tcp_nexus] = asyncRespClient;
        respSock->open();
    }
}

void
AmAsyncXdiResponse::handshakeComplete(boost::shared_ptr<apis::RequestId>& requestId,
                       boost::shared_ptr<int32_t>& port) {
    serverPort = *port;
    XDICLIENTCALL(handshakeComplete(requestId));
}

void
AmAsyncXdiResponse::attachVolumeResp(const Error &error,
                                     boost::shared_ptr<apis::RequestId>& requestId,
                                     boost::shared_ptr<VolumeDesc>& volDesc,
                                     boost::shared_ptr<fpi::VolumeAccessMode>& mode) {
    if (!error.ok()) {
        boost::shared_ptr<fpi::ErrorCode> errorCode(
            boost::make_shared<fpi::ErrorCode>());
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        XDICLIENTCALL(completeExceptionally(requestId, errorCode, message));
    } else {
        XDICLIENTCALL(attachVolumeResponse(requestId, mode));
    }
}

void
AmAsyncXdiResponse::startBlobTxResp(const Error &error,
                                    boost::shared_ptr<apis::RequestId>& requestId,
                                    boost::shared_ptr<apis::TxDescriptor>& txDesc) {
    if (!error.ok()) {
        boost::shared_ptr<fpi::ErrorCode> errorCode(
            boost::make_shared<fpi::ErrorCode>());
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        XDICLIENTCALL(completeExceptionally(requestId, errorCode, message));
    } else {
        XDICLIENTCALL(startBlobTxResponse(requestId, txDesc));
    }
}

void
AmAsyncXdiResponse::abortBlobTxResp(const Error &error,
                                    boost::shared_ptr<apis::RequestId>& requestId) {
    if (!error.ok()) {
        boost::shared_ptr<fpi::ErrorCode> errorCode(
            boost::make_shared<fpi::ErrorCode>());
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        XDICLIENTCALL(completeExceptionally(requestId, errorCode, message));
    } else {
        XDICLIENTCALL(abortBlobTxResponse(requestId));
    }
}

void
AmAsyncXdiResponse::commitBlobTxResp(const Error &error,
                                     boost::shared_ptr<apis::RequestId>& requestId) {
    if (!error.ok()) {
        boost::shared_ptr<fpi::ErrorCode> errorCode(
            boost::make_shared<fpi::ErrorCode>());
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        XDICLIENTCALL(completeExceptionally(requestId, errorCode, message));
    } else {
        XDICLIENTCALL(commitBlobTxResponse(requestId));
    }
}

void
AmAsyncXdiResponse::updateBlobResp(const Error &error,
                                   boost::shared_ptr<apis::RequestId>& requestId) {
    if (!error.ok()) {
        boost::shared_ptr<fpi::ErrorCode> errorCode(
            boost::make_shared<fpi::ErrorCode>());
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        XDICLIENTCALL(completeExceptionally(requestId, errorCode, message));
    } else {
        XDICLIENTCALL(updateBlobResponse(requestId));
    }
}

void
AmAsyncXdiResponse::updateBlobOnceResp(const Error &error,
                                       boost::shared_ptr<apis::RequestId>& requestId) {
    if (!error.ok()) {
        boost::shared_ptr<fpi::ErrorCode> errorCode(
            boost::make_shared<fpi::ErrorCode>());
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        XDICLIENTCALL(completeExceptionally(requestId, errorCode, message));
    } else {
        XDICLIENTCALL(updateBlobOnceResponse(requestId));
    }
}

void
AmAsyncXdiResponse::updateMetadataResp(const Error &error,
                                       boost::shared_ptr<apis::RequestId>& requestId) {
    if (!error.ok()) {
        boost::shared_ptr<fpi::ErrorCode> errorCode(
            boost::make_shared<fpi::ErrorCode>());
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        XDICLIENTCALL(completeExceptionally(requestId, errorCode, message));
    } else {
        XDICLIENTCALL(updateMetadataResponse(requestId));
    }
}

void
AmAsyncXdiResponse::deleteBlobResp(const Error &error,
                                   boost::shared_ptr<apis::RequestId>& requestId) {
    if (!error.ok()) {
        boost::shared_ptr<fpi::ErrorCode> errorCode(
            boost::make_shared<fpi::ErrorCode>());
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        XDICLIENTCALL(completeExceptionally(requestId, errorCode, message));
    } else {
        XDICLIENTCALL(deleteBlobResponse(requestId));
    }
}

void
AmAsyncXdiResponse::statBlobResp(const Error &error,
                                 boost::shared_ptr<apis::RequestId>& requestId,
                                 boost::shared_ptr<fpi::BlobDescriptor>& blobDesc) {
    if (!error.ok()) {
        boost::shared_ptr<fpi::ErrorCode> errorCode(
            boost::make_shared<fpi::ErrorCode>());
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        if (ERR_CAT_ENTRY_NOT_FOUND == error) {
            *errorCode = fpi::MISSING_RESOURCE;
        }
        XDICLIENTCALL(completeExceptionally(requestId, errorCode, message));
    } else {
        XDICLIENTCALL(statBlobResponse(requestId, blobDesc));
    }
}

void
AmAsyncXdiResponse::volumeStatusResp(const Error &error,
                                     boost::shared_ptr<apis::RequestId>& requestId,
                                     boost::shared_ptr<apis::VolumeStatus>& volumeStatus) {
    if (!error.ok()) {
        boost::shared_ptr<fpi::ErrorCode> errorCode(
            boost::make_shared<fpi::ErrorCode>());
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        if (ERR_CAT_ENTRY_NOT_FOUND == error) {
            *errorCode = fpi::MISSING_RESOURCE;
        }
        XDICLIENTCALL(completeExceptionally(requestId, errorCode, message));
    } else {
        XDICLIENTCALL(volumeStatus(requestId, volumeStatus));
    }
}

void
AmAsyncXdiResponse::volumeContentsResp(const Error &error,
                                       boost::shared_ptr<apis::RequestId>& requestId,
                                       boost::shared_ptr<
                                       std::vector<fpi::BlobDescriptor>>& volContents) {
    if (!error.ok()) {
        boost::shared_ptr<fpi::ErrorCode> errorCode(
            boost::make_shared<fpi::ErrorCode>());
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        if (ERR_CAT_ENTRY_NOT_FOUND == error) {
            *errorCode = fpi::MISSING_RESOURCE;
        }
        XDICLIENTCALL(completeExceptionally(requestId, errorCode, message));
    } else {
        XDICLIENTCALL(volumeContents(requestId, volContents));
    }
}

void
AmAsyncXdiResponse::setVolumeMetadataResp(const Error &error,
                                          boost::shared_ptr<apis::RequestId>& requestId) {
    if (!error.ok()) {
        boost::shared_ptr<fpi::ErrorCode> errorCode(
            boost::make_shared<fpi::ErrorCode>());
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        if (ERR_CAT_ENTRY_NOT_FOUND == error) {
            *errorCode = fpi::MISSING_RESOURCE;
        }
        XDICLIENTCALL(completeExceptionally(requestId, errorCode, message));
    } else {
        XDICLIENTCALL(setVolumeMetadataResponse(requestId));
    }
}

void
AmAsyncXdiResponse::getVolumeMetadataResp(const Error &error,
                                          boost::shared_ptr<apis::RequestId>& requestId,
                                          api_type::shared_meta_type& metadata) {
    if (!error.ok()) {
        boost::shared_ptr<fpi::ErrorCode> errorCode(
            boost::make_shared<fpi::ErrorCode>());
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        if (ERR_CAT_ENTRY_NOT_FOUND == error) {
            *errorCode = fpi::MISSING_RESOURCE;
        }
        XDICLIENTCALL(completeExceptionally(requestId, errorCode, message));
    } else {
        XDICLIENTCALL(getVolumeMetadataResponse(requestId, metadata));
    }
}

void
AmAsyncXdiResponse::getBlobResp(const Error &error,
                                boost::shared_ptr<apis::RequestId>& requestId,
                                const boost::shared_ptr<std::vector<boost::shared_ptr<std::string>>>& bufs,
                                int& length) {
    static auto empty_buffer = boost::make_shared<std::string>(0, 0x00);
    if (!error.ok()) {
        boost::shared_ptr<fpi::ErrorCode> errorCode(
            boost::make_shared<fpi::ErrorCode>());
        *errorCode = (error == ERR_BLOB_NOT_FOUND ? fpi::MISSING_RESOURCE :
                                                    fpi::BAD_REQUEST);
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        XDICLIENTCALL(completeExceptionally(requestId, errorCode, message));
    } else {
        // TODO(bszmyd): Mon 27 Apr 2015 06:17:05 PM MDT
        // When Xdi and AmProc support vectored reads, return the whole
        // vector, not just the front element. For now assume one object.
        // A nullptr (with ERR_OK), indicates a zero'd out object
        auto& buf = (bufs && !bufs->empty()) ? bufs->front() : empty_buffer;
        XDICLIENTCALL(getBlobResponse(requestId, buf));
    }
}

void
AmAsyncXdiResponse::getBlobWithMetaResp(const Error &error,
                                        boost::shared_ptr<apis::RequestId>& requestId,
                                        const boost::shared_ptr<std::vector<boost::shared_ptr<std::string>>>& bufs,
                                        int& length,
                                        boost::shared_ptr<fpi::BlobDescriptor>& blobDesc) {
    static auto empty_buffer = boost::make_shared<std::string>(0, 0x00);
    if (!error.ok()) {
        boost::shared_ptr<fpi::ErrorCode> errorCode(
            boost::make_shared<fpi::ErrorCode>());
        *errorCode = (error == ERR_BLOB_NOT_FOUND ? fpi::MISSING_RESOURCE :
                                                    fpi::BAD_REQUEST);
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        XDICLIENTCALL(completeExceptionally(requestId, errorCode, message));
    } else {
        // TODO(bszmyd): Mon 27 Apr 2015 06:17:05 PM MDT
        // When Xdi and AmProc support vectored reads, return the whole
        // vector, not just the front element. For now assume one object.
        // A nullptr (with ERR_OK), indicates a zero'd out object
        auto& buf = (bufs && !bufs->empty()) ? bufs->front() : empty_buffer;
        XDICLIENTCALL(getBlobWithMetaResponse(requestId, buf, blobDesc));
    }
}

boost::shared_ptr<fpi::BlobDescriptor>
transform_descriptor(boost::shared_ptr<fds::BlobDescriptor> descriptor) {
    auto retBlobDesc = boost::make_shared<fpi::BlobDescriptor>();
    retBlobDesc->name = descriptor->getBlobName();
    retBlobDesc->byteCount = descriptor->getBlobSize();

    for (const_kv_iterator it = descriptor->kvMetaBegin();
         it != descriptor->kvMetaEnd();
         ++it) {
        retBlobDesc->metadata[it->first] = it->second;
    }
    return retBlobDesc;
}
}  // namespace fds
