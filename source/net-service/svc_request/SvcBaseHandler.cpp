/* Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>

#include <net/BaseAsyncSvcHandler.h>
#include <util/Log.h>
#include <net/SvcRequest.h>
#include <net/SvcRequestTracker.h>
#include <net/SvcRequestPool.h>
#include <net/SvcMgr.h>
#include <concurrency/SynchronizedTaskExecutor.hpp>
#include <util/fiu_util.h>

namespace fds
{
    /**
     *
     */
    BaseAsyncSvcHandler::BaseAsyncSvcHandler(CommonModuleProviderIf *provider)
    : HasModuleProvider(provider),
    Module("BaseAsyncSvcHandler")
    {
    }

    /**
     *
     */
    BaseAsyncSvcHandler::~BaseAsyncSvcHandler()
    {
    }

    int BaseAsyncSvcHandler::mod_init(SysParams const *const param)
    {
        return 0;
    }

    void BaseAsyncSvcHandler::mod_startup()
    {
    }

    void BaseAsyncSvcHandler::mod_shutdown()
    {
    }

    /**
    * @brief 
    *
    * @param taskExecutor
    */
    void BaseAsyncSvcHandler::setTaskExecutor(SynchronizedTaskExecutor<uint64_t>  *taskExecutor)
    {
        taskExecutor_ = taskExecutor;
    }

    /**
     * @brief
     *
     * @param _return
     * @param msg
     */
    void BaseAsyncSvcHandler::uuidBind(fpi::AsyncHdr &_return, const fpi::UuidBindMsg& msg)
    {
    }

    /**
     * @brief
     *
     * @param _return
     * @param msg
     */
    void BaseAsyncSvcHandler::uuidBind(fpi::AsyncHdr &_return,
                                       boost::shared_ptr<fpi::UuidBindMsg>& msg)
    {
    }

    void BaseAsyncSvcHandler::asyncReqt(const FDS_ProtocolInterface::AsyncHdr& header,
                                        const std::string& payload)
    {
    }

    /**
     * @brief Invokes registered request handler
     *
     * @param header
     * @param payload
     */
    void BaseAsyncSvcHandler::asyncReqt(boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>& header,
                                        boost::shared_ptr<std::string>& payload)
    {
        SVCPERF(header->rqRcvdTs = util::getTimeStampNanos());
        fiu_do_on("svc.uturn.asyncreqt", header->msg_code = ERR_INVALID;
                  sendAsyncResp(*header, fpi::EmptyMsgTypeId, fpi::EmptyMsg()); return; );
        GLOGDEBUG << logString(*header);

        try
        {
            /* Deserialize the message and invoke the handler.  Deserialization is performed
             * by deserializeFdspMsg().
             * For detaails see macro REGISTER_FDSP_MSG_HANDLER()
             */
            fds_assert(header->msg_type_id != fpi::UnknownMsgTypeId);
            asyncReqHandlers_.at(header->msg_type_id) (header, payload);
        }
        catch(std::out_of_range &e)
        {
            fds_assert(!"Unregistered fdsp message type");
            LOGWARN << "Unknown message type: " << static_cast<int32_t>(header->msg_type_id)
                    << " Ignoring";
        }
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
    void BaseAsyncSvcHandler::asyncResp(boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>& header,
                                        boost::shared_ptr<std::string>& payload)
    {
        SVCPERF(header->rspRcvdTs = util::getTimeStampNanos());
        fiu_do_on("svc.disable.schedule", asyncRespHandler(\
            MODULEPROVIDER()->getSvcMgr()->getSvcRequestTracker(), header, payload); return; );
        // fiu_do_on("svc.use.lftp", asyncResp2(header, payload); return; );


        GLOGDEBUG << logString(*header);

        fds_assert(header->msg_type_id != fpi::UnknownMsgTypeId);

        /* Execute on synchronized task exector so that handling for requests
         * with same id gets serialized
         */
        taskExecutor_->schedule(header->msg_src_id,
                               std::bind(&BaseAsyncSvcHandler::asyncRespHandler,
                                         MODULEPROVIDER()->getSvcMgr()->getSvcRequestTracker(),
                                         header, payload));
    }

#if 0
    void BaseAsyncSvcHandler::asyncResp2(
        boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>& header,
        boost::shared_ptr<std::string>& payload)
    {
        auto    workerTp = gSvcRequestPool->getSvcWorkerThreadpool();

        GLOGDEBUG << logString(*header);

        fds_assert(header->msg_type_id != fpi::UnknownMsgTypeId);

        /* Execute on synchronized task exector so that handling for requests
         * with same id gets serialized
         */
        workerTp->scheduleWithAffinity(header->msg_src_id, &BaseAsyncSvcHandler::asyncRespHandler,
                                       header,
                                       payload);
    }
#endif

    /**
     * @brief Static async response handler. Making this static so that this function
     * is accessible locally(with in the process) to everyone.
     *
     * @param header
     * @param payload
     */
    void BaseAsyncSvcHandler::asyncRespHandler(SvcRequestTracker* reqTracker,
        boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>& header,
        boost::shared_ptr<std::string>& payload)
    {
        auto asyncReq = reqTracker->getSvcRequest(static_cast<SvcRequestId>(header->msg_src_id));
        if (!asyncReq)
        {
            GLOGWARN << logString(*header) << " Request doesn't exist (timed out?)";
            return;
        }

        SVCPERF(asyncReq->ts.rqRcvdTs = header->rqRcvdTs);
        SVCPERF(asyncReq->ts.rqHndlrTs = header->rqHndlrTs);
        SVCPERF(asyncReq->ts.rspSerStartTs = header->rspSerStartTs);
        SVCPERF(asyncReq->ts.rspSendStartTs = header->rspSendStartTs);
        SVCPERF(asyncReq->ts.rspRcvdTs = header->rspRcvdTs);

        asyncReq->handleResponse(header, payload);
    }

    void BaseAsyncSvcHandler::sendAsyncResp_(const fpi::AsyncHdr& reqHdr,
                        const fpi::FDSPMsgTypeId &msgTypeId,
                        StringPtr &payload)
    {
        auto respHdr = boost::make_shared<fpi::AsyncHdr>(
            std::move(SvcRequestPool::swapSvcReqHeader(reqHdr)));
        respHdr->msg_type_id = msgTypeId;

        MODULEPROVIDER()->getSvcMgr()->sendAsyncSvcRespMessage(respHdr, payload);
    }
}  // namespace fds
