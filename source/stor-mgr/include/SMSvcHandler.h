/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_SMSVCHANDLER_H_
#define SOURCE_STOR_MGR_INCLUDE_SMSVCHANDLER_H_

#include <fdsp/svc_types_types.h>
#include <net/PlatNetSvcHandler.h>
#include <fdsp/SMSvc.h>

namespace FDS_ProtocolInterface {
struct CtrlNotifyDMTUpdate;
}

namespace fds {

/* Forward declarations */
class SmIoGetObjectReq;
class SmIoPutObjectReq;
class SmIoDeleteObjectReq;
class SmIoAddObjRefReq;
class SmIoNotifyDLTClose;
class SmIoAbortMigration;
class MockSvcHandler;

class SMSvcHandler : virtual public fpi::SMSvcIf, public PlatNetSvcHandler {
  public:
    boost::shared_ptr <MockSvcHandler> mockHandler;
    uint64_t mockTimeoutUs = 200;
    bool mockTimeoutEnabled = false;

    explicit SMSvcHandler(CommonModuleProviderIf *provider);

    virtual int mod_init(SysParams const *const param) override;

    void asyncReqt(boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>& header,
                   boost::shared_ptr<std::string>& payload) override;

    void getObject(const fpi::AsyncHdr &asyncHdr,
            const fpi::GetObjectMsg &getObjMsg) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void putObject(const fpi::AsyncHdr &asyncHdr,
            const fpi::PutObjectMsg &putObjMsg) {
        // Don't do anything here. This stub is just to keep cpp compiler happy
    }

    void getObject(boost::shared_ptr <fpi::AsyncHdr> &asyncHdr,
                   boost::shared_ptr <fpi::GetObjectMsg> &getObjMsg);

    void getObjectCb(boost::shared_ptr <fpi::AsyncHdr> &asyncHdr,
                     const Error &err,
                     SmIoGetObjectReq *read_data);

    void mockGetCb(boost::shared_ptr <fpi::AsyncHdr> &asyncHdr);

    void putObject(boost::shared_ptr <fpi::AsyncHdr> &asyncHdr,
                   boost::shared_ptr <fpi::PutObjectMsg> &putObjMsg);

    void putObjectCb(boost::shared_ptr <fpi::AsyncHdr> &asyncHdr,
                     const Error &err,
                     SmIoPutObjectReq *put_req);

    void mockPutCb(boost::shared_ptr <fpi::AsyncHdr> &asyncHdr);

    void deleteObject(boost::shared_ptr <fpi::AsyncHdr> &asyncHdr,
                      boost::shared_ptr <fpi::DeleteObjectMsg> &deleteObjMsg);

    void deleteObjectCb(boost::shared_ptr <fpi::AsyncHdr> &asyncHdr,
                        const Error &err,
                        SmIoDeleteObjectReq *del_req);

    virtual void
    notifySvcChange(boost::shared_ptr <fpi::AsyncHdr> &hdr,
                    boost::shared_ptr <fpi::NodeSvcInfo> &msg);

    virtual void
    NotifyAddVol(boost::shared_ptr <fpi::AsyncHdr> &hdr,
                 boost::shared_ptr <fpi::CtrlNotifyVolAdd> &vol_msg);

    virtual void
    NotifyRmVol(boost::shared_ptr <fpi::AsyncHdr> &hdr,
                boost::shared_ptr <fpi::CtrlNotifyVolRemove> &vol_msg);

    virtual void
    NotifyModVol(boost::shared_ptr <fpi::AsyncHdr> &hdr,
                 boost::shared_ptr <fpi::CtrlNotifyVolMod> &vol_msg);

    virtual void
    NotifyScavenger(boost::shared_ptr <fpi::AsyncHdr> &hdr,
                    boost::shared_ptr <fpi::CtrlNotifyScavenger> &vol_msg);

    void queryScavengerStatus(boost::shared_ptr <fpi::AsyncHdr> &hdr,
                              boost::shared_ptr <fpi::CtrlQueryScavengerStatus> &query_msg);

    void queryScavengerProgress(boost::shared_ptr <fpi::AsyncHdr> &hdr,
                                boost::shared_ptr <fpi::CtrlQueryScavengerProgress> &query_msg);

    void setScavengerPolicy(boost::shared_ptr <fpi::AsyncHdr> &hdr,
                            boost::shared_ptr <fpi::CtrlSetScavengerPolicy> &policy_msg);

    void queryScavengerPolicy(boost::shared_ptr <fpi::AsyncHdr> &hdr,
                              boost::shared_ptr <fpi::CtrlQueryScavengerPolicy> &policy_msg);

    void queryScrubberStatus(boost::shared_ptr <fpi::AsyncHdr> &hdr,
                             boost::shared_ptr <fpi::CtrlQueryScrubberStatus> &scrub_msg);

    void setScrubberStatus(boost::shared_ptr <fpi::AsyncHdr> &hdr,
                           boost::shared_ptr <fpi::CtrlSetScrubberStatus> &scrub_msg);

    virtual void
    NotifyDLTUpdate(boost::shared_ptr <fpi::AsyncHdr> &hdr,
                    boost::shared_ptr <fpi::CtrlNotifyDLTUpdate> &dlt);


    void NotifyDLTClose(boost::shared_ptr <fpi::AsyncHdr> &hdr,
                        boost::shared_ptr <fpi::CtrlNotifyDLTClose> &dlt);

    void NotifyDLTCloseCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                          const Error &err,
                          SmIoNotifyDLTClose *DLTCloseReq);

    /**
    * Handler for the new DMT messages
    */
    void NotifyDMTUpdate(boost::shared_ptr <fpi::AsyncHdr> &hdr,
                         boost::shared_ptr <fpi::CtrlNotifyDMTUpdate> &dmt);

    void startHybridTierCtrlr(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                              boost::shared_ptr<fpi::CtrlStartHybridTierCtrlrMsg> &hbtMsg);

    void addObjectRef(boost::shared_ptr <fpi::AsyncHdr> &asyncHdr,
                      boost::shared_ptr <fpi::AddObjectRefMsg> &addObjRefMsg);

    void addObjectRefCb(boost::shared_ptr <fpi::AsyncHdr> &asyncHdr,
                        const Error &err,
                        SmIoAddObjRefReq *addObjRefReq);

    void shutdownSM(boost::shared_ptr <fpi::AsyncHdr> &asyncHdr,
                    boost::shared_ptr <fpi::PrepareForShutdownMsg> &shutdownMsg);

    /**
    * Handler for the new SM token migration messages
    * This is a message handler that receives a new DLT message from OM
    */
    void migrationInit(boost::shared_ptr <fpi::AsyncHdr> &asyncHdr,
                       boost::shared_ptr <fpi::CtrlNotifySMStartMigration> &migrationMsg);

    void startMigrationCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                          fds_uint64_t dltVersion,
                          const Error &err);

    void migrationAbort(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                        boost::shared_ptr<fpi::CtrlNotifySMAbortMigration>& abortMsg);

    void migrationAbortCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                          const Error &err,
                          SmIoAbortMigration *abortMigrationReq);


    void initiateObjectSync(boost::shared_ptr <fpi::AsyncHdr> &asyncHdr,
                            boost::shared_ptr <fpi::CtrlObjectRebalanceFilterSet> &filterObjSet);

    void syncObjectSet(boost::shared_ptr <fpi::AsyncHdr> &asyncHdr,
                       boost::shared_ptr <fpi::CtrlObjectRebalanceDeltaSet> &deltaObjSet);

    void getMoreDelta(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                      boost::shared_ptr<fpi::CtrlGetSecondRebalanceDeltaSet>& getDeltaSetMsg);

    void finishClientTokenResync(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                 boost::shared_ptr<fpi::CtrlFinishClientTokenResyncMsg>& finishClientResyncMsg);


    /**
     * Handlers for smcheck
     */
    void NotifySMCheck(boost::shared_ptr<fpi::AsyncHdr>& hdr,
                       boost::shared_ptr<fpi::CtrlNotifySMCheck>& msg);
    void querySMCheckStatus(boost::shared_ptr<fpi::AsyncHdr>& hdr,
                            boost::shared_ptr<fpi::CtrlNotifySMCheckStatus>& msg);

    /**
     * Handlers for ObjectStore control
     */
    void objectStoreCtrl(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                        boost::shared_ptr<fpi::ObjectStoreCtrlMsg>& msg);
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_SMSVCHANDLER_H_
