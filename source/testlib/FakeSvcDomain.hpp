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
struct FakeOm;
struct FakeSvcDomain;
struct FakeSvcFactory;


/**
* @brief Holds collection of FakeSvcs.  FakeSvc provides basic service layer shell.
* This is mostly useful for writing unit tests.
* Some notes:
* -Provides functionality for spawning, killing services.
* -Each service referred by the index as opposed to service uuid for ease of use
* -Index zero always refers to fake om service.
*/
struct FakeSvcDomain {
    FakeSvcDomain(const std::string &configFile);

    virtual void registerService(const fpi::SvcInfo &svcInfo) = 0;

    virtual void getSvcMap(std::vector<fpi::SvcInfo> &svcMap);

    virtual void kill(int idx) = 0;

    virtual void spawn(int idx) = 0;

    virtual void broadcastSvcMap() = 0;

    virtual bool checkSvcInfoAgainstDomain(int svcIdx);

    virtual void updateDmt(const SHPTR<DMT> &dmt) = 0;

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

    FakeSvc*& operator[](int idx) {
        /* For not don't allow access to fake om */
        return svcs_[idx];
    }

    std::vector<fpi::SvcUuid> getSvcUuids() const {
        std::vector<fpi::SvcUuid> svcs;
        std::for_each(svcArray_.begin(),
                      svcArray_.end(),
                      [&svcs](const fpi::SvcInfo &info) { svcs.push_back(info.svc_id.svc_uuid); });
        return svcs;
    }
    fpi::SvcUuid getFakeSvcUuid(int32_t idx) const {
        fds_assert(idx < static_cast<int>(svcArray_.size()));
        return svcArray_[idx].svc_id.svc_uuid;
    }

    /* Fake OM info is set when om registers */
    fpi::SvcUuid                omSvcUuid;
    int                         omPort;

    std::string                 configFile_;
    /* Indexed by easy to remember sequential #s as opposed SvcUuids */
    std::vector<fpi::SvcInfo>   svcArray_;
    /* Reference to svc processes, indexed by sequential #s */
    std::vector<FakeSvc*>       svcs_;
    /* Table to lookup SvcUuid to index */
    std::unordered_map<fpi::SvcUuid, int, SvcUuidHash> idxTbl_;
    FakeSvcFactory              *svcFactory_;
};

/**
* @brief Factory that generates FakeSvc objects
*/
struct FakeSvcFactory {
    FakeSvcFactory(const std::string &configFile)
        : configFile_(configFile)
    {}
    virtual ~FakeSvcFactory() {}
    virtual FakeSvc* generateFakeSvc(FakeSvcDomain *domain, int idx) = 0;

    /**
    * @brief  Helper function to generate platform uuid.  For a a given idx
    * should alwasy return same platform uuid
    */
    fpi::SvcUuid generateFakePlatformUuid(int idx) {
        /* For now don't allow access to fake om */
        fds_assert(idx != 0);
        fpi::SvcUuid uuid;
        uuid.svc_uuid = (PMUUID_BASE + idx) << PMBITS;
        return uuid;
    }
    /**
    * @brief  Helper function to generate platform port.  For a a given idx
    * should alwasy return same platform port 
    */
    int generateFakePlatformPort(int idx) {
        /* For now don't allow access to fake om */
        fds_assert(idx != 0);
        return PMPORT_BASE + (idx * 100);
    }

    std::string                 configFile_;

    static int                  PMBITS;
    static uint64_t             PMUUID_BASE;
    static int                  PMPORT_BASE;
    static fpi::SvcUuid         INVALID_SVCUUID;
};
int FakeSvcFactory::PMBITS = 8;
uint64_t FakeSvcFactory::PMUUID_BASE = 0x100;
int FakeSvcFactory::PMPORT_BASE = 10000;
fpi::SvcUuid FakeSvcFactory::INVALID_SVCUUID;

/**
* @brief Factory that generates only Fake pm objects
*/
template<class FakeSvcT>
struct DefaultSvcFactory : FakeSvcFactory {
    using FakeSvcFactory::FakeSvcFactory;
    FakeSvc* generateFakeSvc(FakeSvcDomain *domain, int idx) override;
};

/**
* @brief Fake service domain
* Provides functionlity for simulating bunch of services in a single process address
* space.  All the communication is synchronous i.e updating service map, dmt, etc. are
* synchronously rather than via async messsages for ease of implementation.
*/
struct FakeSyncSvcDomain : FakeSvcDomain {
    FakeSyncSvcDomain(int numSvcs, const std::string &configFile);
    FakeSyncSvcDomain(int numSvcs,
                      const std::string &configFile,
                      FakeSvcFactory *factory);
    ~FakeSyncSvcDomain();

    virtual void registerService(const fpi::SvcInfo &svcInfo) override;

    virtual void kill(int idx) override;

    virtual void spawn(int idx) override;

    virtual void broadcastSvcMap() override;

    virtual void updateDmt(const SHPTR<DMT> &dmt);

};
using FakeSyncSvcDomainPtr = boost::shared_ptr<FakeSyncSvcDomain>;

/**
* @brief Provides basic service shell for testing.  User should be able to provide
* service message handlers via updateMsgHandler()
*/
struct FakeSvc : SvcProcess {
    FakeSvc();
    FakeSvc(FakeSvcDomain *domain,
            const std::string &configFile,
            const std::string &svcName,
            fds_int64_t platformUuid,
            int platformPort);
    ~FakeSvc();
    virtual void registerSvcProcess() override;
    virtual int run() {return 0;}
    void updateMsgHandler(const fpi::FDSPMsgTypeId msgId,
                          const PlatNetSvcHandler::FdspMsgHandler &handler);

    SHPTR<net::FileTransferService> filetransfer;

    static std::string getFakeSvcName() { return "pm"; }

 protected:
    FakeSvcDomain                       *domain_;
    std::vector<std::string>            args_;
};

/**
* @brief Basic Fake om
*/
struct FakeOm : FakeSvc {
    FakeOm(FakeSvcDomain *domain, const std::string &configFile);
};



FakeSvcDomain::FakeSvcDomain(const std::string &configFile)
{
    configFile_ = configFile;
}

void FakeSvcDomain::getSvcMap(std::vector<fpi::SvcInfo> &svcMap) {
    svcMap = svcArray_;
}

bool FakeSvcDomain::checkSvcInfoAgainstDomain(int svcIdx) {
    fpi::SvcInfo svcInfo = svcArray_[svcIdx];
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
    fds_assert(srcIdx != 0);
    fds_assert(destIdx != 0);

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
    fds_assert(srcIdx != 0);

    auto srcMgr = svcs_[srcIdx]->getSvcMgr();
    DltTokenGroupPtr tokGroup = boost::make_shared<DltTokenGroup>(destIdxs.size());
    int i = 0;
    for (auto &idx : destIdxs) {
        fds_assert(idx != 0);
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
    fds_assert(srcIdx != 0);

    auto srcMgr = svcs_[srcIdx]->getSvcMgr();
    DltTokenGroupPtr tokGroup = boost::make_shared<DltTokenGroup>(destIdxs.size());
    int i = 0;
    for (auto &idx : destIdxs) {
        fds_assert(idx != 0);
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
    fds_assert(srcIdx != 0);

    std::vector<fpi::SvcUuid> primarySvcs;
    std::vector<fpi::SvcUuid> optionalSvcs;
    for (auto &idx : primaryIdxs) {
        fds_assert(idx != 0);
        primarySvcs.push_back(getFakeSvcUuid(idx));
    }
    for (auto &idx : optionalIdxs) {
        fds_assert(idx != 0);
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

template <class FakeSvcT>
FakeSvc* DefaultSvcFactory<FakeSvcT>::generateFakeSvc(FakeSvcDomain *domain, int idx)
{
    return new FakeSvcT(domain,
                       configFile_,
                       FakeSvcT::getFakeSvcName(),
                       generateFakePlatformUuid(idx).svc_uuid,
                       generateFakePlatformPort(idx));
}

FakeSyncSvcDomain::FakeSyncSvcDomain(int numSvcs, const std::string &configFile)
: FakeSyncSvcDomain(numSvcs, configFile, new DefaultSvcFactory<FakeSvc>(configFile))
{
}

FakeSyncSvcDomain::FakeSyncSvcDomain(int numSvcs,
                                     const std::string &configFile,
                                     FakeSvcFactory *factory)
: FakeSvcDomain(configFile)
{
    FakeSvcFactory::INVALID_SVCUUID.svc_uuid = FakeSvcFactory::PMUUID_BASE - 1;

    svcFactory_ = factory;

    /* Create svc mgr instances */
    svcs_.resize(numSvcs);

    /* Create OM instance first */
    // TODO(Rao): Don't hard code platform.conf
    svcs_[0] = new FakeOm(this, "platform.conf");
    svcs_[0]->registerSvcProcess();

    for (uint32_t i = 1; i < svcs_.size(); i++) {
        auto fakeSvc = svcFactory_->generateFakeSvc(this, i);
        svcs_[i] = fakeSvc;
        idxTbl_[fakeSvc->getSvcMgr()->getSelfSvcUuid()] = i;
        svcs_[i]->registerSvcProcess();
    }
}

FakeSyncSvcDomain::~FakeSyncSvcDomain() {
    for (auto &s : svcs_) {
        if (s) {
            delete s;
        }
    }
    delete svcFactory_;
}

void FakeSyncSvcDomain::registerService(const fpi::SvcInfo &svcInfo) {
    uint64_t idx = static_cast<uint64_t>(idxTbl_[svcInfo.svc_id.svc_uuid]);
    fds_assert(idx < svcs_.size());
    if (idx >= svcArray_.size()) {
        svcArray_.resize(idx+1);
    }
    svcArray_[idx] = svcInfo;
    broadcastSvcMap();
}

void FakeSyncSvcDomain::kill(int idx)
{
    fds_verify(idx != 0);

    delete svcs_[idx];
    svcs_[idx] = nullptr;
}

void FakeSyncSvcDomain::spawn(int idx)
{
    fds_verify(idx != 0);

    /* FakeSvc constructor will call registerService() agains FakeSyncSvcDomain.
     * which will update the service map and broadcast svcmap.
     * This is done to close follow how service registration takes place with OM.
     * NOTE: The registration, updating service map, and propagating around the
     * domain takes place synchronously
     */
    svcs_[idx] = svcFactory_->generateFakeSvc(this, idx);
    svcs_[idx]->registerSvcProcess();
}

void FakeSyncSvcDomain::broadcastSvcMap()
{
    for (auto &s : svcs_) {
        if (s) {
            s->getSvcMgr()->updateSvcMap(svcArray_);
        }
    }
}

void FakeSyncSvcDomain::updateDmt(const SHPTR<DMT> &dmt)
{
    Error e;

    std::string dmtData;
    dmt->getSerialized(dmtData);

    for (auto &s : svcs_) {
        if (s) {
            Error e = s->getSvcMgr()->getDmtManager()->addSerializedDMT(dmtData,
                                                                        nullptr,
                                                                        DMT_COMMITTED);
            fds_verify(e == ERR_OK);
        }
    }
}

FakeSvc::FakeSvc()
{
}

FakeSvc::FakeSvc(FakeSvcDomain *domain, 
                 const std::string &configFile,
                 const std::string &svcName,
                 fds_int64_t platformUuid,
                 int platformPort) {
    /* Putting this log statetement will create a logger */
    GLOGNORMAL << "Constructing fakesvc domain";

    domain_ = domain;

    // TODO(Rao): Move towards using initAsModule_() provided by SvcProcess
    /* config */
    std::string configBasePath = "fds." + svcName + ".";
    boost::shared_ptr<FdsConfig> config(new FdsConfig(configFile, 0, {nullptr}));
    conf_helper_.init(config, configBasePath);
    config->set("fds.pm.platform_uuid", std::to_string(platformUuid));
    config->set("fds.pm.platform_port", platformPort);
    svcName_ = svcName;

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
    filetransfer = SHPTR<net::FileTransferService>(new net::FileTransferService(std::string("/tmp/ft-") + std::to_string(platformUuid),
                                                                                getSvcMgr() ));
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

void FakeSvc::updateMsgHandler(const fpi::FDSPMsgTypeId msgId,
                               const PlatNetSvcHandler::FdspMsgHandler &handler)
{
    auto reqHandler = getSvcMgr()->getSvcRequestHandler();
    reqHandler->updateHandler(msgId, handler);
}

FakeOm::FakeOm(FakeSvcDomain *domain, const std::string &configFile)
{
    domain_ = domain;

    auto handler = boost::make_shared<PlatNetSvcHandler>(this);
    auto processor = boost::make_shared<fpi::PlatNetSvcProcessor>(handler);

    args_.push_back("om");
    args_.push_back("--fds.om.threadpool.num_threads=2");

    init((int) args_.size(),
         (char**) &(args_[0]),
         true,
         configFile,
         "fds.om.",
         "",
         nullptr,
         handler,
         processor);

    domain->omSvcUuid = getSvcMgr()->getSelfSvcUuid();
    domain->omPort = getSvcMgr()->getSvcPort();
}

}  // namespace fds

#endif   // SOURCE_INCLUDE_FAKESVC_DOMAIN_H_
