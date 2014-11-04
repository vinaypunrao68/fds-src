/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <vector>
#include <AmAsyncResponseApi.h>

#include <arpa/inet.h>
#include <thrift/concurrency/ThreadManager.h>
#include <thrift/concurrency/PosixThreadFactory.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/server/TNonblockingServer.h>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/transport/TServerSocket.h>
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

AmAsyncXdiResponse::AmAsyncXdiResponse()
        : serverIp("127.0.0.1"),
          serverPort(9876) {
    // Set client to unitialized
    asyncRespClient.reset();
}

AmAsyncXdiResponse::~AmAsyncXdiResponse() {
}

void
AmAsyncXdiResponse::initiateClientConnect() {
    // Setup the async response client
    boost::shared_ptr<xdi_att::TTransport> respSock(
        boost::make_shared<xdi_att::TSocket>(serverIp,
                                             serverPort));
    boost::shared_ptr<xdi_att::TFramedTransport> respTrans(
        boost::make_shared<xdi_att::TFramedTransport>(respSock));
    boost::shared_ptr<xdi_atp::TProtocol> respProto(
        boost::make_shared<xdi_atp::TBinaryProtocol>(respTrans));
    asyncRespClient = boost::make_shared<apis::AsyncAmServiceResponseClient>(respProto);
    respSock->open();
}

void
AmAsyncXdiResponse::attachVolumeResp(const Error &error,
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
AmAsyncXdiResponse::statBlobResp(const Error &error,
                                 boost::shared_ptr<apis::RequestId>& requestId,
                                 boost::shared_ptr<apis::BlobDescriptor>& blobDesc) {
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
                                       std::vector<apis::BlobDescriptor>>& volContents) {
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
                                 char* buf) {
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
         boost::shared_ptr<std::string> response =
                 boost::make_shared<std::string>(buf);
        XDICLIENTCALL(asyncRespClient, getBlobResponse(requestId, response));
    }
}
}  // namespace fds
