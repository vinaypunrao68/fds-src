/* Copyright 2015 Formation Data Systems, Inc.
 */

#include <vector>
#include <string>
#include <sstream>
#include <concurrency/Mutex.h>
#include <arpa/inet.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <fdsp/PlatNetSvc.h>
#include <fdsp/OMSvc.h>
#include <net/fdssocket.h>
#include <fdsp/fds_service_types.h>
#include <fdsp_utils.h>
#include <fds_module_provider.h>
#include <net/SvcRequestPool.h>
#include <net/SvcServer.h>
#include <net/SvcMgr.h>

namespace fds {

std::size_t SvcUuidHash::operator()(const fpi::SvcUuid& svcId) const {
    return svcId.svc_uuid;
}

template<class T>
boost::shared_ptr<T> allocRpcClient(const std::string ip, const int &port,
                                    const bool &blockOnConnect)
{
    auto sock = bo::make_shared<net::Socket>(ip, port);
    auto trans = bo::make_shared<tt::TFramedTransport>(sock);
    auto proto = bo::make_shared<tp::TBinaryProtocol>(trans);
    boost::shared_ptr<T> client = bo::make_shared<T>(proto);
    if (!blockOnConnect) {
        /* This may throw an exception when trying to open.  Caller is expected to handle
         * the exception
         */
        sock->open();
    } else {
        sock->connect();
    }
    return client;
}

SvcMgr::SvcMgr(fpi::PlatNetSvcProcessorPtr processor, const fpi::SvcInfo &svcInfo)
    : Module("SvcMgr")
{
    auto config = gModuleProvider->get_conf_helper();
    omIp_ = config.get_abs<std::string>("fds.common.om_ip_list");
    omPort_ = config.get_abs<int>("fds.common.om_port");
    omSvcUuid_.svc_uuid = static_cast<int64_t>(config.get_abs<long long>("fds.common.om_uuid"));

    svcInfo_ = svcInfo;
    // svcUuid_.svc_uuid= static_cast<int64_t>(config.get<long long>("svc.uuid"));
    // port_ = config.get<int>("svc.port");
    /* Create the server */
    svcServer_ = boost::make_shared<SvcServer>(svcInfo_.svc_port, processor);

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

void SvcMgr::sendAsyncSvcMessage(fpi::AsyncHdrPtr &header,
                                 StringPtr &payload)
{
    SvcHandlePtr svcHandle;
    fpi::SvcUuid &svcUuid = header->msg_dst_uuid;

    do {
        fds_scoped_lock lock(svcHandleMapLock_);
        if (!getSvcHandle_(svcUuid, svcHandle)) {
            GLOGWARN << "Svc handle for svcuuid: " << svcUuid.svc_uuid << " not found";
            break;
        }
    } while (false);

    if (svcHandle) {
        svcHandle->sendAsyncSvcMessage(header, payload);
    } else {
        postSvcSendError(header);
    }
}

void SvcMgr::broadcastasyncSvcMessage(fpi::AsyncHdrPtr &h,
                                      StringPtr &payload,
                                      const SvcInfoPredicate& predicate)
{
    SvcHandleMap map;

    {
        /* Make a copy of the service map */
        fds_scoped_lock lock(svcHandleMapLock_);
        map = svcHandleMap_;
    }

    for (auto &kv : map) {
        /* Create header for the target */
        auto header = boost::make_shared<fpi::AsyncHdr>(*h);
        header->msg_dst_uuid = kv->first;

        kv->second->sendAsyncSvcMessageOnPredicate(header, payload, predicate);
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
    return svcInfo_.svc_id.svc_uuid;
}

int SvcMgr::getSvcPort() const
{
    return svcInfo_.svc_port;
}

void SvcMgr::getOmIPPort(std::string &omIp, int &port)
{
    omIp = omIp_;
    port = omPort_;
}

fpi::OMSvcClientPtr SvcMgr::getNewOMSvcClient() const
{
    fpi::OMSvcClientPtr omClient;
    while (true) {
        try {
            omClient = allocRpcClient<fpi::OMSvcClient>(omIp_, omPort_, true);
            break;
        } catch (std::exception &e) {
            GLOGWARN << "allocRpcClient failed.  Exception: " << e.what()
                << ".  ip: "  << omIp_ << " port: " << omPort_;
        } catch (...) {
            GLOGWARN << "allocRpcClient failed.  Unknown exception. ip: "
                << omIp_ << " port: " << omPort_;
        }
    }

    return omClient;
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

void SvcHandle::sendAsyncSvcMessage(fpi::AsyncHdrPtr &header,
                                    StringPtr &payload)
{
    bool bSendSuccess = true;
    {
        fds_scoped_lock lock(lock_);
        bSendSuccess  = sendAsyncSvcMessageCommon_(header, payload);
    }
    if (!bSendSuccess) {
        gModuleProvider->getSvcMgr()->postSvcSendError(header);
    }
}

void SvcHandle::sendAsyncSvcMessageOnPredicate(fpi::AsyncHdrPtr &header,
                                               StringPtr &payload,
                                               const SvcInfoPredicate& predicate)
{
    bool bSendSuccess = true;
    {
        fds_scoped_lock lock(lock_);
        /* Only send to services matching predicate */
        if (predicate(svcInfo_)) {
            bSendSuccess  = sendAsyncSvcMessageCommon_(header, payload);
        }
    }

    if (!bSendSuccess) {
        gModuleProvider->getSvcMgr()->postSvcSendError(header);
    }
}

bool SvcHandle::sendAsyncSvcMessageCommon_(fpi::AsyncHdrPtr &header,
                                           StringPtr &payload)
{
    /* NOTE: This code assumes lock is held */

    if (isSvcDown_()) {
        /* No point trying to send when service is down */
        return false
    }
    try {
        if (!svcClient_) {
            svcClient_ = allocRpcClient<fpi::PlatNetSvcClient>(svcInfo_.ip,
                                                               svcInfo_.svc_port,
                                                               false);
        }
        svcClient_->asyncReqt(*header, *payload);
        return true;
    } catch (std::exception &e) {
        GLOGWARN << "allocRpcClient failed.  Exception: " << e.what() << ".  "  << header;
        bSendFailed = true;
        markSvcDown_();
    } catch (...) {
        GLOGWARN << "allocRpcClient failed.  Unknown exception." << header;
        bSendFailed = true;
        markSvcDown_();
    }
    return true;
}

void SvcHandle::updateSvcHandle(const fpi::SvcInfo &newInfo)
{
    GLOGDEBUG << "Incoming update: " << fds::logString(newInfo);

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
    return fds::logString(svcInfo_);
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
