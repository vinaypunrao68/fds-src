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

DmCheckerHandler::DmCheckerHandler(DataMgr& dataManager)
    : Handler(dataManager)
{
}

void DmCheckerHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                        boost::shared_ptr<fpi::DmMigrationChkMsg>& message) {
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*message));

}

void DmCheckerHandler::handleQueueItem(dmCatReq* dmRequest) {
}

void DmCheckerHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                         boost::shared_ptr<fpi::DmMigrationChkMsg>& message,
                                         Error const& e, dmCatReq* dmRequest) {
}

}  // namespace dm
}  // namespace fds
