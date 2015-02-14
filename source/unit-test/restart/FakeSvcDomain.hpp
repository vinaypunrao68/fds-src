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

namespace fds {
struct FakeSyncSvcDomain;
struct FakeSvc;
struct FakeSvcDomain;

struct FakeSvcDomain {
    virtual void registerService(const fpi::SvcInfo &svcInfo) = 0;

    virtual void getSvcMap(std::vector<fpi::SvcInfo> &svcMap);

    virtual void kill(int idx) = 0;

    virtual void spawn(int idx) = 0;

    virtual void broadcastSvcMap() = 0;

    virtual bool checkSvcInfoAgainstDomain(int svcIdx);

    virtual void sendGetStatusEpSvcRequest(int srcIdx, int destIdx,
                        SvcRequestCbTask<EPSvcRequest, fpi::GetSvcStatusRespMsg> &cbHandle);

    static fpi::SvcUuid getSvcUuid(int idx) {
        fpi::SvcUuid svcUuid;
        svcUuid.svc_uuid = SVCUUID_BASE + idx;
        return svcUuid;
    }

    static uint64_t SVCUUID_BASE;
    static int PORT_BASE;
    static fpi::SvcUuid INVALID_SVCUUID;

    std::vector<fpi::SvcInfo> svcMap_;
    std::vector<FakeSvc*> svcs_;
};
uint64_t FakeSvcDomain::SVCUUID_BASE = 0x100;
int FakeSvcDomain::PORT_BASE = 10000;
fpi::SvcUuid FakeSvcDomain::INVALID_SVCUUID;


/**
* @brief Fake service domain
* Provides functionlity for simulating bunch of services in a single process address
* space.  All the communication is synchronous.
*/
struct FakeSyncSvcDomain : FakeSvcDomain {
    FakeSyncSvcDomain(int numSvcs);
    ~FakeSyncSvcDomain();

    virtual void registerService(const fpi::SvcInfo &svcInfo) override;

    virtual void kill(int idx) override;

    virtual void spawn(int idx) override;

    virtual void broadcastSvcMap() override;

};
using FakeSyncSvcDomainPtr = boost::shared_ptr<FakeSyncSvcDomain>;

struct FakeSvc : SvcProcess {
    FakeSvc(FakeSvcDomain *domain, const std::string &configFile, long long uuid, int port);
    ~FakeSvc();
    virtual void registerSvcProcess() override;
    virtual int run() {return 0;}

 protected:
    FakeSvcDomain *domain_;
};

void FakeSvcDomain::getSvcMap(std::vector<fpi::SvcInfo> &svcMap) {
    svcMap = svcMap_;
}

bool FakeSvcDomain::checkSvcInfoAgainstDomain(int svcIdx) {
    fpi::SvcInfo svcInfo = svcMap_[svcIdx];
    auto svcUuid = getSvcUuid(svcIdx);
    fds_assert(svcUuid == svcInfo.svc_id.svc_uuid);
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

void FakeSvcDomain::sendGetStatusEpSvcRequest(int srcIdx, int destIdx,
                        SvcRequestCbTask<EPSvcRequest, fpi::GetSvcStatusRespMsg> &cbHandle)
{

    auto srcMgr = svcs_[srcIdx]->getSvcMgr();
    auto destMgr = svcs_[destIdx]->getSvcMgr();
    auto svcStatusMsg = boost::make_shared<fpi::GetSvcStatusMsg>();
    auto asyncReq = srcMgr->getSvcRequestMgr()->newEPSvcRequest(destMgr->getSelfSvcUuid());
    asyncReq->setPayload(FDSP_MSG_TYPEID(fpi::GetSvcStatusMsg), svcStatusMsg);
    asyncReq->setTimeoutMs(1000);
    asyncReq->onResponseCb(cbHandle.cb);
    asyncReq->invoke();
}

FakeSyncSvcDomain::FakeSyncSvcDomain(int numSvcs) {
    uint64_t svcUuidCntr = SVCUUID_BASE;
    int portCntr = PORT_BASE; 

    INVALID_SVCUUID.svc_uuid = svcUuidCntr - 1;

    /* Create svc mgr instances */
    svcs_.resize(numSvcs);
    for (uint32_t i = 0; i < svcs_.size(); i++) {
        svcs_[i] = new FakeSvc(this, "/fds/etc/platform.conf",svcUuidCntr, portCntr);

        svcUuidCntr++;
        portCntr++;
    }

    /* Update the service map on every svcMgr instance */
    for (uint32_t i = 0; i < svcs_.size(); i++) {
        auto svcMgr = svcs_[i]->getSvcMgr();
        svcMgr->updateSvcMap(svcMap_);
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
    uint64_t idx = svcInfo.svc_id.svc_uuid.svc_uuid - SVCUUID_BASE;
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
    svcs_[idx] = new FakeSvc(this, "/fds/etc/platform.conf",
                           SVCUUID_BASE + idx,
                           PORT_BASE + idx);
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
                 long long uuid, int port) {
    domain_ = domain;

    /* config */
    std::string configBasePath = "fds.dm.";
    boost::shared_ptr<FdsConfig> config(new FdsConfig(configFile, 0, {nullptr}));
    conf_helper_.init(config, configBasePath);
    config->set("fds.dm.svc.uuid", uuid);
    config->set("fds.dm.svc.port", port);

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
