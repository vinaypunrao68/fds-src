/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <DataMgr.h>
#include <dmhandler.h>
#include <DMSvcHandler.h>

namespace fds {
namespace dm {

SimpleHandler::SimpleHandler(DataMgr& dataManager) : Handler(dataManager) {
    if (!dataManager.features.isTestModeEnabled()) {
        REGISTER_DM_MSG_HANDLER(fpi::StartRefScanMsg, handleStartRefScanRequest);
    }
}

void SimpleHandler::handleStartRefScanRequest(ASYNC_HANDLER_PARAMS(StartRefScanMsg)) {
    dataManager.refCountMgr->scanActiveObjects();
}

}  // namespace dm
}  // namespace fds
