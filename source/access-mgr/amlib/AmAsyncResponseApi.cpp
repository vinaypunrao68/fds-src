/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
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
        asyncRespClient->completeExceptionally(requestId,
                                               errorCode,
                                               message);
    } else {
        asyncRespClient->startBlobTxResponse(requestId,
                                             txDesc);
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
        asyncRespClient->completeExceptionally(requestId,
                                               errorCode,
                                               message);
    } else {
        asyncRespClient->abortBlobTxResponse(requestId);
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
        asyncRespClient->completeExceptionally(requestId,
                                               errorCode,
                                               message);
    } else {
        asyncRespClient->commitBlobTxResponse(requestId);
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
        asyncRespClient->completeExceptionally(requestId,
                                               errorCode,
                                               message);
    } else {
        asyncRespClient->updateBlobOnceResponse(requestId);
    }
}

}  // namespace fds
