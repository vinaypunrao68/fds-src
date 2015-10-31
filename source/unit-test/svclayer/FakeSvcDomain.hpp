/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FAKESVC_DOMAIN_H_
#define SOURCE_INCLUDE_FAKESVC_DOMAIN_H_

#include <string>
#include <boost/make_shared.hpp>
#include <fds_module_provider.h>
#include <fds_process.h>
#include <net/SvcMgr.h>
#include <net/SvcProcess.h>
#include <fdsp_utils.h>
#include <net/filetransferservice.h>

namespace fds {
struct FakeSyncSvcDomain;
struct FakeSvc;
struct FakeSvcDomain;



struct FakeSvcDomain {
    FakeSvcDomain(const std::string &configFile);
    virtual void registerService(const fpi::SvcInfo &svcInfo) = 0;

    virtual void getSvcMap(std::vector<fpi::SvcInfo> &svcMap);

    virtual void kill(int idx) = 0;

    virtual void spawn(int idx) = 0;

    virtual void broadcastSvcMap() = 0;

    virtual bool checkSvcInfoAgainstDomain(int svcIdx);

    virtual EPSvcRequestPtr sendGetStatusEpSvcRequest(int srcIdx, int destIdx,
                        SvcRequestCbTask<EPSvcRequest, fpi::GetSvcStatusRespMsg> &cbHandle);
    virtual FailoverSvcRequestPtr sendGetStatusFailoverSvcRequest(
        int srcIdx,
        const std::vector<int> &destIdxs,
        SvcRequestCbTask<FailoverSvcRequest, fpi::GetSvcStatusRespMsg> &cbHandle);
    virtual QuorumSvcRequestPtr sendGetStatusQuorumSvcRequest(
        int srcIdx, const std::vector<int> &destIdxs,
        SvcRequestCbTask<QuorumSvcRequest, fpi::GetSvcStatusRespMsg> &cbHandle);
    virtual MultiPrimarySvcRequestPtr sendGetStatusMultiPrimarySvcRequest(
        int srcIdx,
        const std::vector<int> &primaryIdxs,
        const std::vector<int> &optionalIdxs,
        MultiPrimarySvcRequestCbTask &cbHandle);

    FakeSvc*& operator[](int idx) {return svcs_[idx];}

    virtual fpi::SvcUuid getFakeSvcUuid(int idx) {
        fpi::SvcUuid uuid;
        uuid.svc_uuid = (PMUUID_BASE + idx) << PMBITS;
        return uuid;
    }
    virtual int getFakeSvcPort(int idx) {
        return PMPORT_BASE + (idx * 100);
    }
    virtual int getFakeSvcIdx(const fpi::SvcUuid &uuid) {
        return (uuid.svc_uuid >> PMBITS) - PMUUID_BASE;
    }

    static int PMBITS;
    static uint64_t PMUUID_BASE;
    static int PMPORT_BASE;
    static fpi::SvcUuid INVALID_SVCUUID;

    std::string configFile_;
    std::vector<fpi::SvcInfo> svcMap_;
    std::vector<FakeSvc*> svcs_;
};
int FakeSvcDomain::PMBITS = 8;
uint64_t FakeSvcDomain::PMUUID_BASE = 0x100;
int FakeSvcDomain::PMPORT_BASE = 10000;
fpi::SvcUuid FakeSvcDomain::INVALID_SVCUUID;

/**
* @brief Fake service domain
* Provides functionlity for simulating bunch of services in a single process address
* space.  All the communication is synchronous.
*/
struct FakeSyncSvcDomain : FakeSvcDomain {
    FakeSyncSvcDomain(int numSvcs, const std::string &configFile);
    ~FakeSyncSvcDomain();

    virtual void registerService(const fpi::SvcInfo &svcInfo) override;

    virtual void kill(int idx) override;

    virtual void spawn(int idx) override;

    virtual void broadcastSvcMap() override;

};
using FakeSyncSvcDomainPtr = boost::shared_ptr<FakeSyncSvcDomain>;

struct FakeSvc : SvcProcess {
    FakeSvc(FakeSvcDomain *domain, const std::string &configFile, fds_int64_t uuid, int port);
    ~FakeSvc();
    virtual void registerSvcProcess() override;
    virtual int run() {return 0;}
    SHPTR<net::FileTransferService> filetransfer;

 protected:
    FakeSvcDomain *domain_;
};

FakeSvcDomain::FakeSvcDomain(const std::string &configFile) {
    configFile_ = configFile;
}

void FakeSvcDomain::getSvcMap(std::vector<fpi::SvcInfo> &svcMap) {
    svcMap = svcMap_;
}

bool FakeSvcDomain::checkSvcInfoAgainstDomain(int svcIdx) {
    fpi::SvcInfo svcInfo = svcMap_[svcIdx];
    auto svcUuid = svcInfo.svc_id.svc_uuid;
    for (auto &s : svcs_) {
        if (!s) {
            /* ignore when service is down or when s is my service */
            continue;
        }
        fpi::SvcInfo targetInfo;
        bool ret = s->getSvcMgr()->getSvcInfo(svcUuid, targetInfo);
        if (!ret || (svcInfo != targetInfo)) {
            return false;
        }
    }
    return true;
}

EPSvcRequestPtr FakeSvcDomain::sendGetStatusEpSvcRequest(int srcIdx, int destIdx,
                    SvcRequestCbTask<EPSvcRequest, fpi::GetSvcStatusRespMsg> &cbHandle)
{

    auto srcMgr = svcs_[srcIdx]->getSvcMgr();
    auto svcStatusMsg = boost::make_shared<fpi::GetSvcStatusMsg>();
    auto asyncReq = srcMgr->getSvcRequestMgr()->newEPSvcRequest(getFakeSvcUuid(destIdx));
    asyncReq->setPayload(FDSP_MSG_TYPEID(fpi::GetSvcStatusMsg), svcStatusMsg);
    asyncReq->setTimeoutMs(1000);
    asyncReq->onResponseCb(cbHandle.cb);
    asyncReq->invoke();
    return asyncReq;
}

FailoverSvcRequestPtr FakeSvcDomain::sendGetStatusFailoverSvcRequest(int srcIdx,
                    const std::vector<int> &destIdxs,
                    SvcRequestCbTask<FailoverSvcRequest, fpi::GetSvcStatusRespMsg> &cbHandle)
{
    auto srcMgr = svcs_[srcIdx]->getSvcMgr();
    DltTokenGroupPtr tokGroup = boost::make_shared<DltTokenGroup>(destIdxs.size());
    int i = 0;
    for (auto &idx : destIdxs) {
        tokGroup->set(i, NodeUuid(getFakeSvcUuid(idx)));
        i++;
    }
    auto epProvider = boost::make_shared<DltObjectIdEpProvider>(tokGroup);

    auto svcStatusMsg = boost::make_shared<fpi::GetSvcStatusMsg>();
    auto asyncReq = srcMgr->getSvcRequestMgr()->newFailoverSvcRequest(epProvider);
    asyncReq->setPayload(FDSP_MSG_TYPEID(fpi::GetSvcStatusMsg), svcStatusMsg);
    asyncReq->setTimeoutMs(1000);
    asyncReq->onResponseCb(cbHandle.cb);
    asyncReq->invoke();
    return asyncReq;
}

QuorumSvcRequestPtr FakeSvcDomain::sendGetStatusQuorumSvcRequest(
    int srcIdx,
    const std::vector<int> &destIdxs,
    SvcRequestCbTask<QuorumSvcRequest, fpi::GetSvcStatusRespMsg> &cbHandle)
{
    auto srcMgr = svcs_[srcIdx]->getSvcMgr();
    DltTokenGroupPtr tokGroup = boost::make_shared<DltTokenGroup>(destIdxs.size());
    int i = 0;
    for (auto &idx : destIdxs) {
        tokGroup->set(i, NodeUuid(getFakeSvcUuid(idx)));
        i++;
    }
    auto epProvider = boost::make_shared<DltObjectIdEpProvider>(tokGroup);

    auto svcStatusMsg = boost::make_shared<fpi::GetSvcStatusMsg>();
    auto asyncReq = srcMgr->getSvcRequestMgr()->newQuorumSvcRequest(epProvider);
    asyncReq->setPayload(FDSP_MSG_TYPEID(fpi::GetSvcStatusMsg), svcStatusMsg);
    asyncReq->setTimeoutMs(1000);
    asyncReq->onResponseCb(cbHandle.cb);
    asyncReq->invoke();
    return asyncReq;
}

MultiPrimarySvcRequestPtr FakeSvcDomain::sendGetStatusMultiPrimarySvcRequest(int srcIdx,
                                                        const std::vector<int> &primaryIdxs,
                                                        const std::vector<int> &optionalIdxs,
                                                        MultiPrimarySvcRequestCbTask &cbHandle)
{
    std::vector<fpi::SvcUuid> primarySvcs;
    std::vector<fpi::SvcUuid> optionalSvcs;
    for (auto &idx : primaryIdxs) {
        primarySvcs.push_back(getFakeSvcUuid(idx));
    }
    for (auto &idx : optionalIdxs) {
        optionalSvcs.push_back(getFakeSvcUuid(idx));
    }

    auto srcMgr = svcs_[srcIdx]->getSvcMgr();

    auto svcStatusMsg = boost::make_shared<fpi::GetSvcStatusMsg>();
    auto asyncReq = srcMgr->getSvcRequestMgr()->newMultiPrimarySvcRequest(
        primarySvcs, optionalSvcs);
    asyncReq->setPayload(FDSP_MSG_TYPEID(fpi::GetSvcStatusMsg), svcStatusMsg);
    asyncReq->setTimeoutMs(1000);
    asyncReq->onPrimariesRespondedCb(
        /* We only care once all endpoints respond*/ 
        [](MultiPrimarySvcRequest*, const Error&, StringPtr) {}
        );
    asyncReq->onAllRespondedCb(cbHandle.cb);
    asyncReq->invoke();
    return asyncReq;
}

FakeSyncSvcDomain::FakeSyncSvcDomain(int numSvcs, const std::string &configFile)
: FakeSvcDomain(configFile)
{
    INVALID_SVCUUID.svc_uuid = PMUUID_BASE - 1;

    /* Create svc mgr instances */
    svcs_.resize(numSvcs);
    for (uint32_t i = 0; i < svcs_.size(); i++) {
        auto uuid = getFakeSvcUuid(i);
        auto port = getFakeSvcPort(i);
        svcs_[i] = new FakeSvc(this, configFile_, uuid.svc_uuid, port);
    }
}

FakeSyncSvcDomain::~FakeSyncSvcDomain() {
    for (auto &s : svcs_) {
        if (s) {
            delete s;
        }
    }
}

void FakeSyncSvcDomain::registerService(const fpi::SvcInfo &svcInfo) {
    uint64_t idx = static_cast<uint64_t>(getFakeSvcIdx(svcInfo.svc_id.svc_uuid));
    fds_assert(idx < svcs_.size());
    if (idx >= svcMap_.size()) {
        svcMap_.resize(idx+1);
    }
    svcMap_[idx] = svcInfo;
    broadcastSvcMap();
}

void FakeSyncSvcDomain::kill(int idx) {
    delete svcs_[idx];
    svcs_[idx] = nullptr;
}

void FakeSyncSvcDomain::spawn(int idx) {
    /* FakeSvc constructor will call registerService() agains FakeSyncSvcDomain.
     * which will update the service map and broadcast svcmap.
     * This is done to close follow how service registration takes place with OM.
     * NOTE: The registration, updating service map, and propagating around the
     * domain takes place synchronously
     */
    svcs_[idx] = new FakeSvc(this, configFile_,
                             getFakeSvcUuid(idx).svc_uuid,
                             getFakeSvcPort(idx));
}

void FakeSyncSvcDomain::broadcastSvcMap() {
    for (auto &s : svcs_) {
        if (s) {
            s->getSvcMgr()->updateSvcMap(svcMap_);
        }
    }
}

FakeSvc::FakeSvc(FakeSvcDomain *domain, 
                 const std::string &configFile,
                 fds_int64_t uuid, int port) {
    /* Putting this log statetement will create a logger */
    GLOGNORMAL << "Constructing fakesvc domain";

    domain_ = domain;

    /* config */
    std::string configBasePath = "fds.pm.";
    boost::shared_ptr<FdsConfig> config(new FdsConfig(configFile, 0, {nullptr}));
    conf_helper_.init(config, configBasePath);
    config->set("fds.pm.platform_uuid", std::to_string(uuid));
    config->set("fds.pm.platform_port", port);
    svcName_ = "pm";

    GetLog()->setSeverityFilter(
        fds_log::getLevelFromName(conf_helper_.get<std::string>("log_severity","DEBUG")));

    /* timer */
    timer_servicePtr_.reset(new FdsTimer());

    /* counters */
    cntrs_mgrPtr_.reset(new FdsCountersMgr(configBasePath));

    /* threadpool */
    proc_thrp = new fds_threadpool(3);

    /* Populate service information */
    setupSvcInfo_();

    /* Set up service layer */
    auto handler = boost::make_shared<PlatNetSvcHandler>(this);
    auto processor = boost::make_shared<fpi::PlatNetSvcProcessor>(handler);
    setupSvcMgr_(handler, processor);
    filetransfer = SHPTR<net::FileTransferService>(new net::FileTransferService(std::string("/tmp/ft-") + std::to_string(uuid),
                                                                                getSvcMgr() ));
    /* Default implementation registers with OM.  Until registration complets
     * this will not return
     */
    registerSvcProcess();
}

FakeSvc::~FakeSvc() {
}

void FakeSvc::registerSvcProcess()
{
    std::vector<fpi::SvcInfo> svcMap;

    domain_->registerService(svcInfo_);
    domain_->getSvcMap(svcMap);
    svcMgr_->updateSvcMap(svcMap);
}

}  // namespace fds

#endif   // SOURCE_INCLUDE_FAKESVC_DOMAIN_H_
