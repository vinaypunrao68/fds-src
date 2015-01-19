/* Copyright 2015 Formation Data Systems, Inc.
 */

#include <vector>
#include <string>
#include <sstream>
#include <concurrency/Mutex.h>
#include <net/SvcMgr.h>
#include <fdsp/fds_service_types.h>

namespace fds {
SvcMgr::SvcMgr()
    : Module("SvcMgr")
{
}

SvcMgr::~SvcMgr()
{
}

int SvcMgr::mod_init(SysParams const *const p)
{
    GLOGNOTIFY;
    return 0;
}

void SvcMgr::mod_startup()
{
    GLOGNOTIFY;
}

void SvcMgr::mod_enable_service()
{
    GLOGNOTIFY;
}

void SvcMgr::mod_shutdown()
{
    GLOGNOTIFY;
}

void SvcMgr::updateSvcMap(const std::vector<fpi::SvcInfo> &entries)
{
    fds_scoped_lock lock(svcHandleMapLock_);
    for (auto &e : entries) {
        auto svcHandleItr = svcHandleMap_.find(e.svc_id.svc_uuid);
        if (svcHandleItr == svcHandleMap_.end()) {
            /* New service handle entry.  Note, we don't allocate rpcClient.  We do this lazily
             * when needed to not incur the cost of socket creation.
             */
            auto svcHandle = boost::make_shared<SvcHandle>(e);
            svcHandleMap_.emplace(std::make_pair(e.svc_id.svc_uuid, svcHandle));
            GLOGDEBUG << "svc update.  svcuuid: " << e.svc_id.svc_uuid.svc_uuid
                    << " is new.  Incarnation: " << e.incarnationNo;
        } else {
            svcHandleItr->second->updateSvcHandle(e);
        }
    }
}

bool SvcMgr::getSvcHandle_(const fpi::SvcUuid &svcUuid, SvcHandlePtr& handle) const
{
    auto itr = svcHandleMap_.find(svcUuid);
    if (itr == svcHandleMap_.end()) {
        return false;
    }
    handle = itr->second;
    return true;
}

void SvcMgr::sendAsyncSvcRequest(const fpi::SvcUuid &svcUuid,
                                 fpi::AsyncHdrPtr &header,
                                 StringPtr &payload)
{
    SvcHandlePtr svcHandle;
    {
        fds_scoped_lock lock(svcHandleMapLock_);
        if (!getSvcHandle_(svcUuid, svcHandle)) {
            GLOGWARN << "Svc handle for svcuuid: " << svcUuid.svc_uuid << " not found";
            fds_panic("handle not found");
            // TODO(Rao): Post an error, if it's tracked message
            return;
        }
    }
    svcHandle->sendAsyncSvcRequest(header, payload);
}

SvcHandle::SvcHandle()
{
}

SvcHandle::SvcHandle(const fpi::SvcInfo &info)
{
    svcInfo_ = info;
    GLOGDEBUG << "Operation: new service handle";
    GLOGDEBUG << logString();
}

SvcHandle::~SvcHandle()
{
    GLOGDEBUG << logString();
}

void SvcHandle::sendAsyncSvcRequest(fpi::AsyncHdrPtr &header,
                                    StringPtr &payload)
{
    fds_scoped_lock lock(lock_);
    // TODO(Rao): Impl
    fds_panic("Not impl");
}

void SvcHandle::updateSvcHandle(const fpi::SvcInfo &newInfo)
{
    GLOGDEBUG << "Incoming update: " << logString(newInfo);

    fds_scoped_lock lock(lock_);
    if (svcInfo_.incarnationNo < newInfo.incarnationNo) {
        /* Update to new incaration information.  Invalidate the old rpc client */
        svcInfo_ = newInfo;
        rpcClient_.reset();
        GLOGDEBUG << "Operation: update to new incarnation";
    } else if (svcInfo_.incarnationNo == newInfo.incarnationNo &&
               newInfo.svc_status == fpi::SVC_STATUS_INACTIVE) {
        /* Mark current incaration inactivnewInfo.  Invalidate the rpc client */
        svcInfo_.svc_status = fpi::SVC_STATUS_INACTIVE; 
        rpcClient_.reset();
        GLOGDEBUG << "Operation: set current incarnation as down";
    } else {
        GLOGDEBUG << "Operation: not applied";
    }

    GLOGDEBUG << "After update: " << logString();
}

std::string SvcHandle::logString()
{
    /* NOTE: not protected by lock */
    return logString(svcInfo_);
}

std::string SvcHandle::logString(const fpi::SvcInfo &info)
{
    std::stringstream ss;
    ss << "Svc handle svc_uuid: " << info.svc_id.svc_uuid.svc_uuid << " ip: " << info.ip
        << " port: " << info.svc_port << " incarnation: " << info.incarnationNo
        << " status: " << info.svc_status;
    return ss.str();
}

}  // namespace fds
