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
}

void DmMigrationHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                        boost::shared_ptr<fpi::CtrlNotifyDMStartMigrationMsg>& message) {
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*message));

}

void DmMigrationHandler::handleQueueItem(dmCatReq* dmRequest) {
}

void DmMigrationHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                         boost::shared_ptr<fpi::CtrlNotifyDMStartMigrationMsg>& message,
                                         Error const& e, dmCatReq* dmRequest) {
}

}  // namespace dm
}  // namespace fds
