/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_OMSVCPROCESS_H_
#define SOURCE_INCLUDE_NET_OMSVCPROCESS_H_

#include <boost/shared_ptr.hpp>
#include <fdsp/svc_api_types.h>
#include <net/SvcMgr.h>
#include <net/PlatNetSvcHandler.h>
#include <net/SvcProcess.h>
#include <net/SvcMgr.h>
#include <net/SvcRequestPool.h>
#include <net/SvcProcess.h>
#include <fdsp_utils.h>


namespace fds {

/**
* @brief TestOm
*/
struct TestOm : SvcProcess {
    TestOm(int argc, char *argv[], bool initAsModule);
    virtual ~TestOm();

    void init(int argc, char *argv[], bool initAsModule);

    virtual void registerSvcProcess() override;

    virtual int run() override;

    /* Messages invoked by handler */
    void registerService(boost::shared_ptr<fpi::SvcInfo>& svcInfo);
    void getSvcMap(std::vector<fpi::SvcInfo> & svcMap);
    void getDLT( ::FDS_ProtocolInterface::CtrlNotifyDLTUpdate& _return, const int64_t nullarg);
    void getDMT( ::FDS_ProtocolInterface::CtrlNotifyDMTUpdate& _return, const int64_t nullarg);
    void getAllVolumeDescriptors(fpi::GetAllVolumeDescriptors& _return, const int64_t nullarg);

 protected:
    /**
    * @brief Sets up configdb used for persistence
    */
    virtual void setupConfigDb_() override;

    fds_mutex svcMapLock_;
    std::unordered_map<fpi::SvcUuid, fpi::SvcInfo, SvcUuidHash> svcMap_;
};

struct TestOmHandler : virtual public fpi::OMSvcIf, public PlatNetSvcHandler {
    explicit TestOmHandler(TestOm *om) 
    : PlatNetSvcHandler(om)
    {
        om_ = om;
    }

    void registerService(const fpi::SvcInfo& svcInfo) {}
    void getSvcInfo(fpi::SvcInfo &_return,
                    const  fpi::SvcUuid& svcUuid) override {}
    void getSvcMap(std::vector<fpi::SvcInfo> & _return, const int64_t nullarg) {}
    void getDLT( ::FDS_ProtocolInterface::CtrlNotifyDLTUpdate& _return, const int64_t nullarg) {}
    void getDMT( ::FDS_ProtocolInterface::CtrlNotifyDMTUpdate& _return, const int64_t nullarg) {}
    void getAllVolumeDescriptors(fpi::GetAllVolumeDescriptors& _return, const int64_t nullarg) {}

    void registerService(boost::shared_ptr<fpi::SvcInfo>& svcInfo) {
        // Your implementation goes here
        om_->registerService(svcInfo);
    }

    void getSvcMap(std::vector<fpi::SvcInfo> & _return, boost::shared_ptr<int64_t>& nullarg) {
        // Your implementation goes here
        om_->getSvcMap(_return);
    }

    void getDLT( ::FDS_ProtocolInterface::CtrlNotifyDLTUpdate& _return, boost::shared_ptr<int64_t>& nullarg) {
    }

    void getDMT( ::FDS_ProtocolInterface::CtrlNotifyDMTUpdate& _return, boost::shared_ptr<int64_t>& nullarg) {
    }

    void getAllVolumeDescriptors(fpi::GetAllVolumeDescriptors& _return, boost::shared_ptr<int64_t>& nullarg) {
    }

    virtual void getSvcInfo(fpi::SvcInfo & _return,
                            boost::shared_ptr< fpi::SvcUuid>& svcUuid) override
    {
    }

    TestOm *om_;
};

TestOm::TestOm(int argc, char *argv[], bool initAsModule)
                               
{
    init(argc, argv, initAsModule);
}

TestOm::~TestOm()
{
}

void TestOm::init(int argc, char *argv[], bool initAsModule)
{
    /* Set up OMsvcProcessor for handling messages */
    boost::shared_ptr<TestOmHandler> handler = boost::make_shared<TestOmHandler>(this);
    boost::shared_ptr<fpi::OMSvcProcessor> processor = boost::make_shared<fpi::OMSvcProcessor>(handler);

    /* Set up process related services such as logger, timer, etc. */
    SvcProcess::init(argc, argv, initAsModule, "platform.conf", "fds.om.",
                     "om.log", nullptr, handler, processor);
}

void TestOm::registerSvcProcess()
{
    LOGNOTIFY;
    /* Add self to svcmap */
    svcMgr_->updateSvcMap({svcInfo_});
}

void TestOm::setupConfigDb_()
{
    LOGNOTIFY;
}

int TestOm::run() {
    LOGNOTIFY << "Doing work";
    readyWaiter.done();
    shutdownGate_.waitUntilOpened();
    return 0;
}

void TestOm::registerService(boost::shared_ptr<fpi::SvcInfo>& svcInfo)
{
    GLOGNOTIFY << "Register service request.  Svcinfo: " << fds::logString(*svcInfo);

    bool updated = false;
    {
        /* admit and update persistent */
        fds_scoped_lock l(svcMapLock_);
        auto mapItr = svcMap_.find(svcInfo->svc_id.svc_uuid);
        if (mapItr == svcMap_.end()) {
            GLOGNOTIFY << "Persisted new service";
            svcMap_.emplace(std::make_pair(svcInfo->svc_id.svc_uuid, *svcInfo));
            updated = true;
        } else if (mapItr->second.incarnationNo < svcInfo->incarnationNo) {
            GLOGNOTIFY << "Persisted service update";
            mapItr->second = *svcInfo;
            updated = true;
        } else {
            GLOGNOTIFY << "Ignored service";
        }
    }

    if (updated) {
        /* Prepare update service map message */
        boost::shared_ptr<std::string>buf;
        auto header = getSvcMgr()->getSvcRequestMgr()->newSvcRequestHeaderPtr(
            SvcRequestPool::SVC_UNTRACKED_REQ_ID,
            FDSP_MSG_TYPEID(fpi::UpdateSvcMapMsg),
            svcMgr_->getSelfSvcUuid(),
            fpi::SvcUuid(),
            DLT_VER_INVALID, 0, 0);
        fpi::UpdateSvcMapMsgPtr updateMsg = boost::make_shared<fpi::UpdateSvcMapMsg>(); 
        updateMsg->updates.push_back(*svcInfo);
        fds::serializeFdspMsg(*updateMsg, buf);

        /* First update svcMgr in OM */
        svcMgr_->updateSvcMap(updateMsg->updates);

        /* Update the domain by broadcasting */
        svcMgr_->broadcastAsyncSvcReqMessage(header, buf,
                                          [](const fpi::SvcInfo& info) {return true;});
        LOGDEBUG << "Broadcasted svcInfo: " << fds::logString(*svcInfo);
    }
}

void TestOm::getSvcMap(std::vector<fpi::SvcInfo> & svcMap)
{
    fds_scoped_lock l(svcMapLock_);
    svcMgr_->getSvcMap(svcMap);
}
}  // namespace fds
#endif  // SOURCE_INCLUDE_NET_OMSVCPROCESS_H_