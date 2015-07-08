/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dmhandler.h>
#include <util/Log.h>
#include <fds_error.h>
#include <DMSvcHandler.h>  // This shouldn't be necessary, included because of
                           // incomplete type errors in PlatNetSvcHandler.h
#include <net/PlatNetSvcHandler.h>
#include <PerfTrace.h>
#include <VolumeMeta.h>
#include <DmIoReq.h>

namespace fds {
namespace dm {

DmMigrationHandler::DmMigrationHandler(DataMgr& dataManager)
    : Handler(dataManager)
{
    if (!dataManager.features.isTestMode()) {
        REGISTER_DM_MSG_HANDLER(fpi::CtrlNotifyDMStartMigrationMsg, handleRequest);
    }
}

void DmMigrationHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                        boost::shared_ptr<fpi::CtrlNotifyDMStartMigrationMsg>& message) {
    LOGMIGRATE << logString(*asyncHdr) << logString(*message);

    auto dmReq = new DmIoMigration(FdsDmSysTaskId, message);
    dmReq->cb = BIND_MSG_CALLBACK(DmMigrationHandler::handleResponse, asyncHdr, message);
    dmReq->localCb = std::bind(&DmMigrationHandler::handleResponseReal, this,
    							std::placeholders::_1, std::placeholders::_2,
								std::placeholders::_3, std::placeholders::_4);
    dmReq->asyncHdrPtr = asyncHdr;

    fds_verify(dmReq->io_vol_id == FdsDmSysTaskId);
    fds_verify(dmReq->io_type == FDS_DM_MIGRATION);

    addToQueue(dmReq);

}

void DmMigrationHandler::handleQueueItem(dmCatReq* dmRequest) {
    QueueHelper helper(dataManager, dmRequest);
    DmIoMigration* typedRequest = static_cast<DmIoMigration*>(dmRequest);

    // Do some bookkeeping
    helper.skipImplicitCb = true;

    // Talk to migration handler.
    dataManager.dmMigrationMgr->startMigration(dmRequest);
}

void DmMigrationHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                         boost::shared_ptr<fpi::CtrlNotifyDMStartMigrationMsg>& message,
                                         Error const& e, dmCatReq* dmRequest)
{
	/**
	 * Do nothing - this should be skipped as skipImplicitCb is set to true
	 */
}

void DmMigrationHandler::handleResponseReal(fpi::AsyncHdrPtr& asyncHdr,
                                         fpi::CtrlNotifyDMStartMigrationMsgPtr& message,
                                         Error const& e, dmCatReq* dmRequest) {

    LOGMIGRATE << logString(*asyncHdr) << logString(*message);

    asyncHdr->msg_code = e.GetErrno();
    DM_SEND_ASYNC_RESP(*asyncHdr, fpi::CtrlNotifyDMStartMigrationRspMsgTypeId, *message);

    delete dmRequest;
}

DmMigrationBlobFilterHandler::DmMigrationBlobFilterHandler(DataMgr& dataManager)
	: Handler(dataManager)
{
    if (!dataManager.features.isTestMode()) {
        REGISTER_DM_MSG_HANDLER(fpi::CtrlNotifyInitialBlobFilterSetMsg, handleRequest);
    }
}

void DmMigrationBlobFilterHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                        boost::shared_ptr<fpi::CtrlNotifyInitialBlobFilterSetMsg>& message) {
    LOGMIGRATE << logString(*asyncHdr) << logString(*message);

    NodeUuid tmpUuid;
    tmpUuid.uuid_set_val(asyncHdr->msg_src_uuid.svc_uuid);
    auto dmReq = new DmIoResyncInitialBlob(FdsDmSysTaskId, message, tmpUuid);
    dmReq->cb = BIND_MSG_CALLBACK(DmMigrationBlobFilterHandler::handleResponse, asyncHdr, message);
    dmReq->localCb = std::bind(&DmMigrationBlobFilterHandler::handleResponseReal, this,
    							std::placeholders::_1, std::placeholders::_2,
								std::placeholders::_3, std::placeholders::_4);

    fds_verify(dmReq->io_vol_id == FdsDmSysTaskId);
    fds_verify(dmReq->io_type == FDS_DM_RESYNC_INIT_BLOB);

    addToQueue(dmReq);

}

void DmMigrationBlobFilterHandler::handleQueueItem(dmCatReq* dmRequest) {
    QueueHelper helper(dataManager, dmRequest);
    DmIoResyncInitialBlob* typedRequest = static_cast<DmIoResyncInitialBlob*>(dmRequest);

    helper.skipImplicitCb = true;

    dataManager.dmMigrationMgr->startMigrationClient(dmRequest);
}

void DmMigrationBlobFilterHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
			boost::shared_ptr<fpi::CtrlNotifyInitialBlobFilterSetMsg>& message,
			Error const& e, dmCatReq* dmRequest) {
	// Empty, using Real below as an async callback
}

void DmMigrationBlobFilterHandler::handleResponseReal(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
			boost::shared_ptr<fpi::CtrlNotifyInitialBlobFilterSetMsg>& message,
			Error const& e, dmCatReq* dmRequest) {

    LOGMIGRATE << logString(*asyncHdr) << logString(*message);

    asyncHdr->msg_code = e.GetErrno();
    DM_SEND_ASYNC_RESP(*asyncHdr, fpi::CtrlNotifyInitialBlobFilterSetMsgTypeId, *message);

    delete dmRequest;
}

DmMigrationDeltaBlobDescHandler::DmMigrationDeltaBlobDescHandler(DataMgr& dataManager)
	: Handler(dataManager)
{
    REGISTER_DM_MSG_HANDLER(fpi::CtrlNotifyDeltaBlobDescMsg, handleRequest);
}

void DmMigrationDeltaBlobDescHandler::handleRequest(fpi::AsyncHdrPtr& asyncHdr,
                                        fpi::CtrlNotifyDeltaBlobDescMsgPtr& message) {
    LOGMIGRATE << logString(*asyncHdr) << logString(*message);

    auto dmReq = new DmIoMigrationDeltaBlobDesc(message);

    fds_verify(dmReq->io_vol_id == FdsDmSysTaskId);
    fds_verify(dmReq->io_type == FDS_DM_MIG_DELTA_BLOBDESC);

    addToQueue(dmReq);
}

void DmMigrationDeltaBlobDescHandler::handleQueueItem(dmCatReq* dmRequest) {
    QueueHelper helper(dataManager, dmRequest);
    helper.skipImplicitCb = true;
    DmIoMigrationDeltaBlobDesc* typedRequest = static_cast<DmIoMigrationDeltaBlobDesc*>(dmRequest);
    // TODO(Rao): Route to migration exector to apply
}

}  // namespace dm
}  // namespace fds
