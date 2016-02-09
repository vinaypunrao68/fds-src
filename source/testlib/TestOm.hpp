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

    void addVolume(const fds_volid_t &volId,
                   const SHPTR<VolumeDesc>& volume,
                   bool broadcast = false);
    void addDmt(const DMTPtr &dmt);

    /* Messages invoked by handler */
    void registerService(boost::shared_ptr<fpi::SvcInfo>& svcInfo);
    void getSvcMap(std::vector<fpi::SvcInfo> & svcMap);
    void getDLT( ::FDS_ProtocolInterface::CtrlNotifyDLTUpdate& _return, const int64_t nullarg);
    void getDMT( ::FDS_ProtocolInterface::CtrlNotifyDMTUpdate& _return, const int64_t nullarg);
    void getAllVolumeDescriptors(fpi::GetAllVolumeDescriptors& _return, const int64_t nullarg);
    Error setVolumeGroupCoordinator(const fpi::SetVolumeGroupCoordinatorMsg &msg);

 protected:
    /**
    * @brief Sets up configdb used for persistence
    */
    virtual void setupConfigDb_() override;

    fds_mutex svcMapLock_;
    std::unordered_map<fpi::SvcUuid, fpi::SvcInfo, SvcUuidHash> svcMap_;

    fds_mutex volumeLock;
    std::unordered_map<fds_volid_t, SHPTR<VolumeDesc>> volumeTbl;
};

struct TestOmHandler : virtual public fpi::OMSvcIf, public PlatNetSvcHandler {
    explicit TestOmHandler(TestOm *om) 
    : PlatNetSvcHandler(om)
    {
        om_ = om;
        REGISTER_FDSP_MSG_HANDLER(fpi::SetVolumeGroupCoordinatorMsg, setVolumeGroupCoordinator);
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

    void getDMT(fpi::CtrlNotifyDMTUpdate& dmt, boost::shared_ptr<int64_t>& nullarg) {
        std::string data_buffer;
    	DMTPtr dp = om_->getSvcMgr()->getCurrentDMT();
        if (!dp) {
            LOGWARN << "No dmt available";
            return;
        }
    	(*dp).getSerialized(data_buffer);

    	fpi::FDSP_DMT_Data_Type fdt;
    	fdt.__set_dmt_data(data_buffer);
    	dmt.__set_dmt_data(fdt);
    	dmt.__set_dmt_version(dp->getVersion());
    }

    void getAllVolumeDescriptors(fpi::GetAllVolumeDescriptors& _return,
                                 boost::shared_ptr<int64_t>& nullarg) {
        om_->getAllVolumeDescriptors(_return, *nullarg);
    }

    virtual void getSvcInfo(fpi::SvcInfo & _return,
                            boost::shared_ptr< fpi::SvcUuid>& svcUuid) override
    {
    }

    void setVolumeGroupCoordinator(SHPTR<fpi::AsyncHdr> &hdr,
                                   SHPTR<fpi::SetVolumeGroupCoordinatorMsg> &msg)
    {
        hdr->msg_code = om_->setVolumeGroupCoordinator(*msg).GetErrno();
        sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::EmptyMsg), fpi::EmptyMsg());
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

void TestOm::addVolume(const fds_volid_t &volId,
                       const SHPTR<VolumeDesc>& volume,
                       bool broadcast)
{
    fds_scoped_lock l(volumeLock);
    volumeTbl[volId] = volume;

    fds_assert(!broadcast); // broadcast not supported
}

void TestOm::addDmt(const DMTPtr &dmt)
{
    std::string dmtData;
    dmt->getSerialized(dmtData);
    Error e = getSvcMgr()->getDmtManager()->addSerializedDMT(dmtData,
                                                             nullptr,
                                                             DMT_COMMITTED);
    fds_verify(e == ERR_OK);
}

void TestOm::getAllVolumeDescriptors(fpi::GetAllVolumeDescriptors& _return,
                                     const int64_t nullarg)
{
    fds_scoped_lock l(volumeLock);
    for (const auto &v : volumeTbl) {
        _return.volumeList.emplace_back();
        auto &volAdd = _return.volumeList.back();
        v.second->toFdspDesc(volAdd.vol_desc);
    }
}

Error TestOm::setVolumeGroupCoordinator(const fpi::SetVolumeGroupCoordinatorMsg &msg)
{
    fds_scoped_lock l(volumeLock);
    auto itr = volumeTbl.find(fds_volid_t(msg.volumeId));
    if (itr == volumeTbl.end()) {
        LOGWARN << "Failed to set coordinator for volume: " << msg.volumeId;
        return ERR_VOL_NOT_FOUND;
    }
    auto &volDescPtr = itr->second;
    auto &volCoordinatorInfo = msg.coordinator;
    volDescPtr->setCoordinatorId(volCoordinatorInfo.id);
    volDescPtr->setCoordinatorVersion(volCoordinatorInfo.version);
    LOGNOTIFY << "Set volume coordinator for volid: " << msg.volumeId
        << " coordinator: " << volCoordinatorInfo.id.svc_uuid;
    return ERR_OK;
}
}  // namespace fds
#endif  // SOURCE_INCLUDE_NET_OMSVCPROCESS_H_
