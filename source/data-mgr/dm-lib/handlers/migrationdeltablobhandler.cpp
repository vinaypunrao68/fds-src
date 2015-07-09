/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dmhandler.h>
#include <DmIoReq.h>
#include <DMSvcHandler.h>  // This shouldn't be necessary, included because of
                           // incomplete type errors in PlatNetSvcHandler.h
#include <net/PlatNetSvcHandler.h>
#include <util/Log.h>
#include <fds_assert.h>

#include <functional>

namespace fds {
namespace dm {

DmMigrationDeltablobHandler::DmMigrationDeltablobHandler(DataMgr& dataManager)
    : Handler(dataManager)
{
    if (!dataManager.features.isTestMode()) {
        REGISTER_DM_MSG_HANDLER(fpi::CtrlNotifyDeltaBlobsMsg, handleRequest);
    }
}

void DmMigrationDeltablobHandler::handleRequest(
        boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
        boost::shared_ptr<fpi::CtrlNotifyDeltaBlobsMsg>& message) {
    auto dmReq = new DmIoMigDeltaBlob(message);

    fds_verify(dmReq->io_type == FDS_DM_MIG_DELT_BLB);

    DBG(LOGMIGRATE << "Enqueued delta blob migration  request " << logString(*asyncHdr)
        << " " << *reinterpret_cast<DmIoMigDeltaBlob*>(dmReq));

    addToQueue(dmReq);

}

void DmMigrationDeltablobHandler::handleQueueItem(dmCatReq* dmRequest) {
    QueueHelper helper(dataManager, dmRequest);
    DmIoMigDeltaBlob* typedRequest = static_cast<DmIoMigDeltaBlob*>(dmRequest);

    LOGMIGRATE << "Sending the delta blob migration dequest to migration Mgr " << *typedRequest;
    /*
     *  revisit this - Migration Migration executor integration 
     */
    dataManager.dmMigrationMgr->applyDeltaObjects(typedRequest);
}

}  // namespace dm
}  // namespace fds
