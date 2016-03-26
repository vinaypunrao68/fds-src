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
    ResourceUUID fromuuid(hdr->msg_src_uuid);
    bool fFromSM = (fpi::FDSP_STOR_MGR == fromuuid.uuid_get_type());
    LOGNORMAL << "refscan request received from:" << SvcMgr::mapToSvcName(fromuuid.uuid_get_type());
    dataManager.refCountMgr->scanActiveObjects(fFromSM);
}

}  // namespace dm
}  // namespace fds
