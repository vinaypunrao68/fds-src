/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <vector>
#include <AmAsyncResponseApi.h>
#include "AmAsyncXdi.h"
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

#define XDICLIENTCALL(client, func)                   \
    try {                                             \
        client->func;                                 \
    } catch(const xdi_att::TTransportException& e) {  \
        client.reset();                               \
        LOGERROR << "Unable to respond to XDI! "      \
                 << "Dropping response and uninitializing the connection!"; \
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
        asyncRespClient = client_it->second;
    } else {
        // Setup the async response client
        boost::shared_ptr<xdi_att::TTransport> respSock(
            boost::make_shared<xdi_att::TSocket>(
                serverIp, serverPort));
        boost::shared_ptr<xdi_att::TFramedTransport> respTrans(
            boost::make_shared<xdi_att::TFramedTransport>(respSock));
        boost::shared_ptr<xdi_atp::TProtocol> respProto(
            boost::make_shared<xdi_atp::TBinaryProtocol>(respTrans));
        asyncRespClient = std::make_shared<apis::AsyncAmServiceResponseClient>(respProto);
        clients[tcp_nexus] = asyncRespClient;
        respSock->open();
    }
}

void
AmAsyncXdiResponse::handshakeComplete(boost::shared_ptr<apis::RequestId>& requestId,
                       boost::shared_ptr<int32_t>& port) {
    serverPort = *port;
    checkClientConnect();
    XDICLIENTCALL(asyncRespClient, handshakeComplete(requestId));
}

void
AmAsyncXdiResponse::attachVolumeResp(const Error &error,
                                     boost::shared_ptr<apis::RequestId>& requestId) {
    if (!error.ok()) {
        boost::shared_ptr<apis::ErrorCode> errorCode(
            boost::make_shared<apis::ErrorCode>());
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        XDICLIENTCALL(asyncRespClient, completeExceptionally(requestId,
                                                             errorCode,
                                                             message));
    } else {
        XDICLIENTCALL(asyncRespClient, attachVolumeResponse(requestId));
    }
}

void
AmAsyncXdiResponse::startBlobTxResp(const Error &error,
                                    boost::shared_ptr<apis::RequestId>& requestId,
                                    boost::shared_ptr<apis::TxDescriptor>& txDesc) {
    checkClientConnect();
    if (!error.ok()) {
        boost::shared_ptr<apis::ErrorCode> errorCode(
            boost::make_shared<apis::ErrorCode>());
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        XDICLIENTCALL(asyncRespClient, completeExceptionally(requestId,
                                                             errorCode,
                                                             message));
    } else {
        XDICLIENTCALL(asyncRespClient, startBlobTxResponse(requestId,
                                                           txDesc));
    }
}

void
AmAsyncXdiResponse::abortBlobTxResp(const Error &error,
                                    boost::shared_ptr<apis::RequestId>& requestId) {
    checkClientConnect();
    if (!error.ok()) {
        boost::shared_ptr<apis::ErrorCode> errorCode(
            boost::make_shared<apis::ErrorCode>());
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        XDICLIENTCALL(asyncRespClient, completeExceptionally(requestId,
                                                             errorCode,
                                                             message));
    } else {
        XDICLIENTCALL(asyncRespClient, abortBlobTxResponse(requestId));
    }
}

void
AmAsyncXdiResponse::commitBlobTxResp(const Error &error,
                                     boost::shared_ptr<apis::RequestId>& requestId) {
    checkClientConnect();
    if (!error.ok()) {
        boost::shared_ptr<apis::ErrorCode> errorCode(
            boost::make_shared<apis::ErrorCode>());
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        XDICLIENTCALL(asyncRespClient, completeExceptionally(requestId,
                                                             errorCode,
                                                             message));
    } else {
        XDICLIENTCALL(asyncRespClient, commitBlobTxResponse(requestId));
    }
}

void
AmAsyncXdiResponse::updateBlobResp(const Error &error,
                                   boost::shared_ptr<apis::RequestId>& requestId) {
    checkClientConnect();
    if (!error.ok()) {
        boost::shared_ptr<apis::ErrorCode> errorCode(
            boost::make_shared<apis::ErrorCode>());
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        XDICLIENTCALL(asyncRespClient, completeExceptionally(requestId,
                                                             errorCode,
                                                             message));
    } else {
        XDICLIENTCALL(asyncRespClient, updateBlobResponse(requestId));
    }
}

void
AmAsyncXdiResponse::updateBlobOnceResp(const Error &error,
                                       boost::shared_ptr<apis::RequestId>& requestId) {
    checkClientConnect();
    if (!error.ok()) {
        boost::shared_ptr<apis::ErrorCode> errorCode(
            boost::make_shared<apis::ErrorCode>());
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        XDICLIENTCALL(asyncRespClient, completeExceptionally(requestId,
                                                             errorCode,
                                                             message));
    } else {
        XDICLIENTCALL(asyncRespClient, updateBlobOnceResponse(requestId));
    }
}

void
AmAsyncXdiResponse::updateMetadataResp(const Error &error,
                                       boost::shared_ptr<apis::RequestId>& requestId) {
    checkClientConnect();
    if (!error.ok()) {
        boost::shared_ptr<apis::ErrorCode> errorCode(
            boost::make_shared<apis::ErrorCode>());
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        XDICLIENTCALL(asyncRespClient, completeExceptionally(requestId,
                                                             errorCode,
                                                             message));
    } else {
        XDICLIENTCALL(asyncRespClient, updateMetadataResponse(requestId));
    }
}

void
AmAsyncXdiResponse::deleteBlobResp(const Error &error,
                                   boost::shared_ptr<apis::RequestId>& requestId) {
    checkClientConnect();
    if (!error.ok()) {
        boost::shared_ptr<apis::ErrorCode> errorCode(
            boost::make_shared<apis::ErrorCode>());
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        XDICLIENTCALL(asyncRespClient, completeExceptionally(requestId,
                                                             errorCode,
                                                             message));
    } else {
        XDICLIENTCALL(asyncRespClient, deleteBlobResponse(requestId));
    }
}

void
AmAsyncXdiResponse::statBlobResp(const Error &error,
                                 boost::shared_ptr<apis::RequestId>& requestId,
                                 boost::shared_ptr<fpi::BlobDescriptor>& blobDesc) {
    checkClientConnect();
    if (!error.ok()) {
        boost::shared_ptr<apis::ErrorCode> errorCode(
            boost::make_shared<apis::ErrorCode>());
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        if (ERR_CAT_ENTRY_NOT_FOUND == error) {
            *errorCode = apis::MISSING_RESOURCE;
        }
        XDICLIENTCALL(asyncRespClient, completeExceptionally(requestId,
                                                             errorCode,
                                                             message));
    } else {
        XDICLIENTCALL(asyncRespClient, statBlobResponse(requestId, blobDesc));
    }
}

void
AmAsyncXdiResponse::volumeStatusResp(const Error &error,
                                     boost::shared_ptr<apis::RequestId>& requestId,
                                     boost::shared_ptr<apis::VolumeStatus>& volumeStatus) {
    checkClientConnect();
    if (!error.ok()) {
        boost::shared_ptr<apis::ErrorCode> errorCode(
            boost::make_shared<apis::ErrorCode>());
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        if (ERR_CAT_ENTRY_NOT_FOUND == error) {
            *errorCode = apis::MISSING_RESOURCE;
        }
        XDICLIENTCALL(asyncRespClient, completeExceptionally(requestId,
                                                             errorCode,
                                                             message));
    } else {
        XDICLIENTCALL(asyncRespClient, volumeStatus(requestId, volumeStatus));
    }
}

void
AmAsyncXdiResponse::volumeContentsResp(const Error &error,
                                       boost::shared_ptr<apis::RequestId>& requestId,
                                       boost::shared_ptr<
                                       std::vector<fpi::BlobDescriptor>>& volContents) {
    checkClientConnect();
    if (!error.ok()) {
        boost::shared_ptr<apis::ErrorCode> errorCode(
            boost::make_shared<apis::ErrorCode>());
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        if (ERR_CAT_ENTRY_NOT_FOUND == error) {
            *errorCode = apis::MISSING_RESOURCE;
        }
        XDICLIENTCALL(asyncRespClient, completeExceptionally(requestId,
                                                             errorCode,
                                                             message));
    } else {
        XDICLIENTCALL(asyncRespClient, volumeContents(requestId, volContents));
    }
}

void
AmAsyncXdiResponse::getBlobResp(const Error &error,
                                boost::shared_ptr<apis::RequestId>& requestId,
                                boost::shared_ptr<std::string> buf,
                                fds_uint32_t& length) {
    checkClientConnect();
    if (!error.ok()) {
        boost::shared_ptr<apis::ErrorCode> errorCode(
            boost::make_shared<apis::ErrorCode>());
        *errorCode = (error == ERR_BLOB_NOT_FOUND ? apis::MISSING_RESOURCE :
                                                    apis::BAD_REQUEST);
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        XDICLIENTCALL(asyncRespClient, completeExceptionally(requestId,
                                                             errorCode,
                                                             message));
    } else {
        buf = buf->size() > length ? boost::make_shared<std::string>(*buf, 0, length)
            : buf;
        XDICLIENTCALL(asyncRespClient, getBlobResponse(requestId, buf));
    }
}

void
AmAsyncXdiResponse::getBlobWithMetaResp(const Error &error,
                                        boost::shared_ptr<apis::RequestId>& requestId,
                                        boost::shared_ptr<std::string> buf,
                                        fds_uint32_t& length,
                                        boost::shared_ptr<fpi::BlobDescriptor>& blobDesc) {
    checkClientConnect();
    if (!error.ok()) {
        boost::shared_ptr<apis::ErrorCode> errorCode(
            boost::make_shared<apis::ErrorCode>());
        *errorCode = (error == ERR_BLOB_NOT_FOUND ? apis::MISSING_RESOURCE :
                                                    apis::BAD_REQUEST);
        boost::shared_ptr<std::string> message(
            boost::make_shared<std::string>());
        XDICLIENTCALL(asyncRespClient, completeExceptionally(requestId,
                                                             errorCode,
                                                             message));
    } else {
        buf = buf->size() > length ? boost::make_shared<std::string>(*buf, 0, length)
            : buf;
        XDICLIENTCALL(asyncRespClient, getBlobWithMetaResponse(requestId, buf, blobDesc));
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
