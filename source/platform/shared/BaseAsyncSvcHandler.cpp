/* Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>

#include <net/BaseAsyncSvcHandler.h>
#include <util/Log.h>
#include <net/RpcRequest.h>
#include <net/AsyncRpcRequestTracker.h>
#include <concurrency/SynchronizedTaskExecutor.hpp>

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
* @brief 
*
* @param _return
* @param msg
*/
void BaseAsyncSvcHandler::uuidBind(FDS_ProtocolInterface::RespHdr& _return,
                    const FDS_ProtocolInterface::UuidBindMsg& msg)
{
}

/**
* @brief 
*
* @param _return
* @param msg
*/
void BaseAsyncSvcHandler::uuidBind(FDS_ProtocolInterface::RespHdr& _return,
                        boost::shared_ptr<FDS_ProtocolInterface::UuidBindMsg>& msg)
{
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
    asyncRespHandler(header, payload);
}


/**
* @brief Static async response handler. Making this static so that this function
* is accessible locally(with in the process) to everyone.
*
* @param header
* @param payload
*/
void BaseAsyncSvcHandler::asyncRespHandler(
        boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>& header,
        boost::shared_ptr<std::string>& payload)
{
    GLOGDEBUG;

    auto asyncReq = gAsyncRpcTracker->\
            getAsyncRpcRequest(static_cast<AsyncRpcRequestId>(header->msg_src_id));
    if (!asyncReq) {
        GLOGWARN << "Request " << header->msg_src_id << " doesn't exist";
        return;
    }

#if 0
    SynchronizedTaskExecutor<uint64_t> taskExecutor(*NetMgr::ep_mgr_thrpool());
    taskExecutor.schedule(1,
                          std::bind(&AsyncRpcRequestIf::handleResponse,
                                    asyncReq.get(), header, payload));
#endif

    asyncReq->handleResponse(header, payload);
}

}  // namespace fds

