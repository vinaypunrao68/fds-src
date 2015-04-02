/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dmhandler.h>
#include <DMSvcHandler.h>  // This shouldn't be necessary, included because of
                           // incomplete type errors in PlatNetSvcHandler.h
#include <net/PlatNetSvcHandler.h>
#include <util/Log.h>
#include <fds_assert.h>
#include <DmIoReq.h>
#include <PerfTrace.h>

namespace fds {
namespace dm {

VolumeCloseHandler::VolumeCloseHandler() {
    if (!dataMgr->features.isTestMode()) {
        REGISTER_DM_MSG_HANDLER(fpi::CloseVolumeMsg, handleRequest);
    }
}

void VolumeCloseHandler::handleRequest(
        boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
        boost::shared_ptr<fpi::CloseVolumeMsg>& message) {
}

void VolumeCloseHandler::handleQueueItem(dmCatReq* dmRequest) {
}

void VolumeCloseHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                              boost::shared_ptr<fpi::CloseVolumeMsg>& message,
                                              Error const& e, dmCatReq* dmRequest) {
}

}  // namespace dm
}  // namespace fds
