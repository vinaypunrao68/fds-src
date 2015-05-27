/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dmhandler.h>
#include <fds_assert.h>
#include <util/Log.h>
#include <DmIoReq.h>
#include <PerfTrace.h>
#include <fds_error.h>
#include <DMSvcHandler.h>

namespace fds {
namespace dm {

DmCheckerHandler::DmCheckerHandler(DataMgr& dataManager)
    : Handler(dataManager)
{
}

void DmCheckerHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                        boost::shared_ptr<fpi::dmMigrationChkMsg>& message) {
    DBG(GLOGDEBUG << logString(*asyncHdr) << logString(*message));

}

void DmCheckerHandler::handleQueueItem(dmCatReq* dmRequest) {
}

void DmCheckerHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                         boost::shared_ptr<fpi::dmMigrationChkMsg>& message,
                                         Error const& e, dmCatReq* dmRequest) {
}

}  // namespace dm
}  // namespace fds
