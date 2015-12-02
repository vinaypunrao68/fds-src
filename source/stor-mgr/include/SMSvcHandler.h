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

    DECL_ASYNC_HANDLER(getObject              , GetObjectMsg);
    DECL_ASYNC_HANDLER(putObject              , PutObjectMsg);
    DECL_ASYNC_HANDLER(deleteObject           , DeleteObjectMsg);
    DECL_ASYNC_HANDLER(notifySvcChange        , NodeSvcInfo);
    DECL_ASYNC_HANDLER(NotifyAddVol           , CtrlNotifyVolAdd);
    DECL_ASYNC_HANDLER(NotifyRmVol            , CtrlNotifyVolRemove);
    DECL_ASYNC_HANDLER(NotifyModVol           , CtrlNotifyVolMod);
    DECL_ASYNC_HANDLER(NotifyScavenger        , CtrlNotifyScavenger);
    DECL_ASYNC_HANDLER(queryScavengerStatus   , CtrlQueryScavengerStatus);
    DECL_ASYNC_HANDLER(queryScavengerProgress , CtrlQueryScavengerProgress);
    DECL_ASYNC_HANDLER(setScavengerPolicy     , CtrlSetScavengerPolicy);
    DECL_ASYNC_HANDLER(queryScavengerPolicy   , CtrlQueryScavengerPolicy);
    DECL_ASYNC_HANDLER(queryScrubberStatus    , CtrlQueryScrubberStatus);
    DECL_ASYNC_HANDLER(setScrubberStatus      , CtrlSetScrubberStatus);
    DECL_ASYNC_HANDLER(NotifyDLTUpdate        , CtrlNotifyDLTUpdate);
    DECL_ASYNC_HANDLER(NotifyDLTClose         , CtrlNotifyDLTClose);
    DECL_ASYNC_HANDLER(NotifyDMTUpdate        , CtrlNotifyDMTUpdate);
    DECL_ASYNC_HANDLER(startHybridTierCtrlr   , CtrlStartHybridTierCtrlrMsg);
    DECL_ASYNC_HANDLER(addObjectRef           , AddObjectRefMsg);
    DECL_ASYNC_HANDLER(shutdownSM             , PrepareForShutdownMsg);
    DECL_ASYNC_HANDLER(migrationInit          , CtrlNotifySMStartMigration);
    DECL_ASYNC_HANDLER(migrationAbort         , CtrlNotifySMAbortMigration);    
    DECL_ASYNC_HANDLER(initiateObjectSync     , CtrlObjectRebalanceFilterSet);
    DECL_ASYNC_HANDLER(syncObjectSet          , CtrlObjectRebalanceDeltaSet);
    DECL_ASYNC_HANDLER(getMoreDelta           , CtrlGetSecondRebalanceDeltaSet);
    DECL_ASYNC_HANDLER(finishClientTokenResync, CtrlFinishClientTokenResyncMsg);
    DECL_ASYNC_HANDLER(NotifySMCheck          , CtrlNotifySMCheck);
    DECL_ASYNC_HANDLER(querySMCheckStatus     , CtrlNotifySMCheckStatus);
    DECL_ASYNC_HANDLER(activeObjects          , ActiveObjectsMsg);

    void getObjectCb(boost::shared_ptr <fpi::AsyncHdr> &asyncHdr,
                     const Error &err,
                     SmIoGetObjectReq *read_data);
    void mockGetCb(boost::shared_ptr <fpi::AsyncHdr> &asyncHdr);
    void putObjectCb(boost::shared_ptr <fpi::AsyncHdr> &asyncHdr,
                     const Error &err,
                     SmIoPutObjectReq *put_req);
    void mockPutCb(boost::shared_ptr <fpi::AsyncHdr> &asyncHdr);
    void deleteObjectCb(boost::shared_ptr <fpi::AsyncHdr> &asyncHdr,
                        const Error &err,
                        SmIoDeleteObjectReq *del_req);
    void migrationAbortCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                          const Error &err,
                          SmIoAbortMigration *abortMigrationReq);
    void startMigrationCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                          fds_uint64_t dltVersion,
                          const Error &err);
    void addObjectRefCb(boost::shared_ptr <fpi::AsyncHdr> &asyncHdr,
                        const Error &err,
                        SmIoAddObjRefReq *addObjRefReq);
    void NotifyDLTCloseCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                          const Error &err,
                          SmIoNotifyDLTClose *DLTCloseReq);
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_SMSVCHANDLER_H_
