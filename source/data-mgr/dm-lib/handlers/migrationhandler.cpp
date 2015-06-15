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

    addToQueue(dmReq);

}

void DmMigrationHandler::handleQueueItem(dmCatReq* dmRequest) {
    QueueHelper helper(dataManager, dmRequest);
    DmIoMigration* typedRequest = static_cast<DmIoMigration*>(dmRequest);

}

void DmMigrationHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                         boost::shared_ptr<fpi::CtrlNotifyDMStartMigrationMsg>& message,
                                         Error const& e, dmCatReq* dmRequest) {

    LOGMIGRATE << logString(*asyncHdr) << logString(*message);

    asyncHdr->msg_code = e.GetErrno();
    DM_SEND_ASYNC_RESP(*asyncHdr, fpi::CtrlNotifyDMStartMigrationRspMsgTypeId, *message);

    delete dmRequest;
}

}  // namespace dm
}  // namespace fds
