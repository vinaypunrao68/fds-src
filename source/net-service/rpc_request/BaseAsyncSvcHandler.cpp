/* Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>

#include <BaseAsyncSvcHandler.h>
#include <util/Log.h>
#include <net/RpcRequest.h>
#include <AsyncRpcRequestTracker.h>

namespace fds {

/**
 *
 */
BaseAsyncSvcHandler::BaseAsyncSvcHandler() {
}

/**
 *
 */
BaseAsyncSvcHandler::~BaseAsyncSvcHandler() {
}

/**
 *
 * @param header
 */
void BaseAsyncSvcHandler::asyncReqt(
        const FDS_ProtocolInterface::AsyncHdr& header) {
}

/**
 *
 * @param header
 */
void BaseAsyncSvcHandler::asyncReqt(
        boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>& header) {
}

/**
 *
 * @param msg
 */
void BaseAsyncSvcHandler::uuidBind(
        const FDS_ProtocolInterface::UuidBindMsg& msg) {
}

/**
 *
 * @param msg
 */
void BaseAsyncSvcHandler::uuidBind(
        boost::shared_ptr<FDS_ProtocolInterface::UuidBindMsg>& msg) {
}
/**
 *
 * @param header
 * @param payload
 */
void BaseAsyncSvcHandler::asyncResp(const FDS_ProtocolInterface::AsyncHdr& header,
        const std::string& payload)
{
}

/**
 * Handler function for async responses
 * @param header
 * @param payload
 */
void BaseAsyncSvcHandler::asyncResp(
        boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>& header,
        boost::shared_ptr<std::string>& payload)
{
    auto asyncReq = gAsyncRpcTracker->\
            getAsyncRpcRequest(static_cast<AsyncRpcRequestId>(header->msg_src_id));
    if (!asyncReq) {
        GLOGWARN << "Request " << header->msg_src_id << " doesn't exist";
        return;
    }
    asyncReq->handleResponse(header, payload);
}
}  // namespace fds

