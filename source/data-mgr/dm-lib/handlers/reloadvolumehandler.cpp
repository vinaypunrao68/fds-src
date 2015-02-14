/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <dmhandler.h>
#include <util/Log.h>
#include <DmIoReq.h>
#include <DMSvcHandler.h>  // This shouldn't be necessary, included because of
                           // incomplete type errors in BaseAsyncSvcHandler.h
#include <net/BaseAsyncSvcHandler.h>
#include <PerfTrace.h>

#include <boost/algorithm/string/replace.hpp>
#include <pcrecpp.h>

#include <map>
#include <string>

using fpi::AsyncHdr;
using fpi::BlobDescriptorListType;
using fpi::FDSP_MetaDataList;
using fpi::ReloadVolumeMsg;
using fpi::ReloadVolumeMsgTypeId;

using boost::replace_all;

using pcrecpp::RE;
using pcrecpp::UTF8;

using std::map;
using std::string;

namespace fds {
namespace dm {

ReloadVolumeHandler::ReloadVolumeHandler() {
    if (!dataMgr->feature.isTestMode()) {
        REGISTER_DM_MSG_HANDLER(ReloadVolumeMsg, handleRequest);
    }
}

void ReloadVolumeHandler::handleRequest(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                        boost::shared_ptr<ReloadVolumeMsg>& message) {
    /**
     * immediately responding to the command as this should happen on a brand new volume
     * and need not be qos'ed
     */
    fds_volid_t volId = message->volId;
    LOGDEBUG << "reloading catalog for vol:" << volId;
    Error err = dataMgr->timeVolCat_->queryIface()->activateCatalog(volId);
    asyncHdr->msg_code = err.GetErrno();
    DM_SEND_ASYNC_RESP(*asyncHdr, ReloadVolumeMsgTypeId, EmptyMsg());
}

void ReloadVolumeHandler::handleQueueItem(dmCatReq* dmRequest) {}

void ReloadVolumeHandler::handleResponse(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                         boost::shared_ptr<ReloadVolumeMsg>& message,
                                         Error const& e, dmCatReq* dmRequest) {
}

}  // namespace dm
}  // namespace fds
