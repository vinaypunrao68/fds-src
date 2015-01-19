/* Copyright 2015 Formation Data Systems, Inc.
 */

#include <vector>
#include <string>
#include <sstream>
#include <concurrency/Mutex.h>
#include <arpa/inet.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <fdsp/PlatNetSvc.h>
#include <net/fdssocket.h>
#include <fdsp/fds_service_types.h>
#include <fdsp_utils.h>
#include <fds_module_provider.h>
#include <net/SvcRequestPool.h>
#include <net/SvcServer.h>
#include <net/SvcMgr.h>

namespace fds {
SvcMgr::SvcMgr(fpi::PlatNetSvcProcessorPtr processor)
    : Module("SvcMgr")
{
    /* Create the server */
    svcUuid_.svc_uuid= gModuleProvider->get_conf_helper().get<int64_t>("svc.uuid");
    port_ = gModuleProvider->get_conf_helper().get<int>("svc.port");
    svcServer_ = boost::make_shared<SvcServer>(port_, processor);

    // TODO(Rao): Don't make this global
    gSvcRequestPool = new SvcRequestPool();

}

SvcMgr::~SvcMgr()
{
    delete gSvcRequestPool;
}

int SvcMgr::mod_init(SysParams const *const p)
{
    GLOGNOTIFY;

    svcServer_->start();

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
    do {
        fds_scoped_lock lock(svcHandleMapLock_);
        if (!getSvcHandle_(svcUuid, svcHandle)) {
            GLOGWARN << "Svc handle for svcuuid: " << svcUuid.svc_uuid << " not found";
            break;
        }
    } while (false);

    if (svcHandle) {
        svcHandle->sendAsyncSvcRequest(header, payload);
    } else {
        postSvcSendError(header);
    }
}

void SvcMgr::postSvcSendError(fpi::AsyncHdrPtr &header)
{
    swapAsyncHdr(header);
    header->msg_code = ERR_SVC_REQUEST_INVOCATION;
    gSvcRequestPool->postError(header);
}

fpi::SvcUuid SvcMgr::getSvcUuid() const
{
    return svcUuid_;
}

int SvcMgr::getSvcPort() const
{
    return port_;
}

fpi::OMSvcClientPtr SvcMgr::getNewOMSvcClient() const
{
    // TODO(Rao): return om client
    fds_panic("Not implemented");
    return nullptr;
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
    bool bSendFailed = false;
    do {
        fds_scoped_lock lock(lock_);
        if (isSvcDown_()) {
            bSendFailed = true;
            break;
        }
        try {
            if (!svcClient_) {
                allocSvcClient_();
            }
        } catch (std::exception &e) {
            GLOGWARN << "Failed to send.  Excepction: " << e.what() << ".  "  << header;
            bSendFailed = true;
            markSvcDown_();
        } catch (...) {
            GLOGWARN << "Failed to send.  Unknown excepction." << header;
            bSendFailed = true;
            markSvcDown_();
        }
    } while (false);

    if (bSendFailed) {
        gModuleProvider->getSvcMgr()->postSvcSendError(header);
    }
}

void SvcHandle::updateSvcHandle(const fpi::SvcInfo &newInfo)
{
    GLOGDEBUG << "Incoming update: " << logString(newInfo);

    fds_scoped_lock lock(lock_);
    if (svcInfo_.incarnationNo < newInfo.incarnationNo) {
        /* Update to new incaration information.  Invalidate the old rpc client */
        svcInfo_ = newInfo;
        svcClient_.reset();
        GLOGDEBUG << "Operation: update to new incarnation";
    } else if (svcInfo_.incarnationNo == newInfo.incarnationNo &&
               newInfo.svc_status == fpi::SVC_STATUS_INACTIVE) {
        /* Mark current incaration inactivnewInfo.  Invalidate the rpc client */
        svcInfo_.svc_status = fpi::SVC_STATUS_INACTIVE; 
        svcClient_.reset();
        GLOGDEBUG << "Operation: set current incarnation as down";
    } else {
        GLOGDEBUG << "Operation: not applied";
    }

    GLOGDEBUG << "After update: " << logString();
}

std::string SvcHandle::logString() const
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

void SvcHandle::allocSvcClient_()
{
    /* NOTE: Assumes this function is invoked under lock */
    auto sock = bo::make_shared<net::Socket>(svcInfo_.ip, svcInfo_.svc_port);
    auto trans = bo::make_shared<tt::TFramedTransport>(sock);
    auto proto = bo::make_shared<tp::TBinaryProtocol>(trans);
    svcClient_ = bo::make_shared<fpi::PlatNetSvcClient>(proto);
    /* This may throw an exception.  Caller is expected to handle the exception */
    sock->open();
}

bool SvcHandle::isSvcDown_() const
{
    /* NOTE: Assumes this function is invoked under lock */
    return svcInfo_.svc_status == fpi::SVC_STATUS_INACTIVE;
}

void SvcHandle::markSvcDown_()
{
    /* NOTE: Assumes this function is invoked under lock */
    svcInfo_.svc_status = fpi::SVC_STATUS_INACTIVE;
    svcClient_.reset();
    GLOGDEBUG << logString();
}

}  // namespace fds
