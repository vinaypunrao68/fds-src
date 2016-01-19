/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <StorMgr.h>
#include <fdsp_utils.h>
#include <fds_assert.h>
#include <SMSvcHandler.h>
#include <string>
#include <net/SvcMgr.h>
#include <net/SvcRequest.h>
#include <fiu-local.h>
#include <fiu-control.h>
#include <random>
#include <chrono>
#include <MockSMCallbacks.h>
#include <net/SvcMgr.h>
#include <net/MockSvcHandler.h>
#include <fds_timestamp.h>
#include <util/path.h>

DECL_EXTERN_OUTPUT_FUNCS(GenericCommandMsg);

namespace fds {

extern ObjectStorMgr    *objStorMgr;
extern std::string logString(const FDS_ProtocolInterface::AddObjectRefMsg& msg);
extern std::string logString(const FDS_ProtocolInterface::DeleteObjectMsg& msg);
extern std::string logString(const FDS_ProtocolInterface::GetObjectMsg& msg);
extern std::string logString(const FDS_ProtocolInterface::PutObjectMsg& msg);

SMSvcHandler::SMSvcHandler(CommonModuleProviderIf *provider)
    : PlatNetSvcHandler(provider)
{
    /* Data paths messages */
    REGISTER_FDSP_MSG_HANDLER(fpi::GetObjectMsg, getObject);
    REGISTER_FDSP_MSG_HANDLER(fpi::PutObjectMsg, putObject);
    REGISTER_FDSP_MSG_HANDLER(fpi::DeleteObjectMsg, deleteObject);
    REGISTER_FDSP_MSG_HANDLER(fpi::AddObjectRefMsg, addObjectRef);

    /* Service map messages */
    REGISTER_FDSP_MSG_HANDLER(fpi::NodeSvcInfo, notifySvcChange);

    /* volume information and policy messages */
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyVolAdd, NotifyAddVol);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyVolRemove, NotifyRmVol);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyVolMod, NotifyModVol);

    /* GC control messages */
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyScavenger, NotifyScavenger);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlQueryScavengerStatus, queryScavengerStatus);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlQueryScavengerProgress, queryScavengerProgress);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlSetScavengerPolicy, setScavengerPolicy);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlQueryScavengerPolicy, queryScavengerPolicy);

    /* scrubber, which is part of GC policy, status messages */
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlSetScrubberStatus, setScrubberStatus);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlQueryScrubberStatus, queryScrubberStatus);

    /* Online smcheck control messages */
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifySMCheck, NotifySMCheck);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifySMCheckStatus, querySMCheckStatus);

    /* hybrid tiering control messages */
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlStartHybridTierCtrlrMsg, startHybridTierCtrlr);

    /* DLT update messages */
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyDLTUpdate, NotifyDLTUpdate);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyDLTClose, NotifyDLTClose);

    /* SM Service shutdown message */
    REGISTER_FDSP_MSG_HANDLER(fpi::PrepareForShutdownMsg, shutdownSM);

    /* token migration messages */
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifySMStartMigration, migrationInit);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifySMAbortMigration, migrationAbort);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlObjectRebalanceFilterSet, initiateObjectSync);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlObjectRebalanceDeltaSet, syncObjectSet);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlGetSecondRebalanceDeltaSet, getMoreDelta);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlFinishClientTokenResyncMsg, finishClientTokenResync);

    /* DMT update messages */
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyDMTUpdate, NotifyDMTUpdate);

    /* Active Object Messages */
    REGISTER_FDSP_MSG_HANDLER(fpi::ActiveObjectsMsg, activeObjects);

    REGISTER_FDSP_MSG_HANDLER(fpi::GenericCommandMsg, genericCommand);

    /* disk-map update message */
    REGISTER_FDSP_MSG_HANDLER(fpi::NotifyDiskMapChange, diskMapChange);
}

int
SMSvcHandler::mod_init(SysParams const *const param)
{
    mockTimeoutEnabled = MODULEPROVIDER()->get_fds_config()->\
                         get<bool>("fds.sm.testing.enable_mocking");
    mockTimeoutUs = MODULEPROVIDER()->get_fds_config()->\
                    get<uint32_t>("fds.sm.testing.mocktimeout");
    if (true == mockTimeoutEnabled) {
        mockHandler.reset(new MockSvcHandler());
    }
    return 0;
}

void
SMSvcHandler::asyncReqt(boost::shared_ptr<FDS_ProtocolInterface::AsyncHdr>& header,
                        boost::shared_ptr<std::string>& payload)
{
    PlatNetSvcHandler::asyncReqt(header, payload);
}

void
SMSvcHandler::migrationInit(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                            boost::shared_ptr<fpi::CtrlNotifySMStartMigration>& migrationMsg)
{
    Error err(ERR_OK);
    LOGDEBUG << "Received Start Migration for target DLT "
             << migrationMsg->DLT_version;

/* (Gurpreet: Fix for ignoring multiple start migration messages from OM. UNTESTED!!)

    const DLT* curDlt = MODULEPROVIDER()->getSvcMgr()->getDltManager()->getDLT();
    fds_uint64_t reqMigrTgtDlt = migrationMsg->DLT_version;
    auto ongoingMigrTgtDlt = objStorMgr->migrationMgr->getTargetDltVersion();
    if (!objStorMgr->migrationMgr->isMigrationIdle() ||
        ((curDlt->getVersion() != DLT_VER_INVALID)  && !curDlt->isClosed())) {
        LOGWARN << "Received start migration for dlt version: " << reqMigrTgtDlt
                << " during ongoing migration for version: " << ongoingMigrTgtDlt
                << ". Ignoring message.";
        if (ongoingMigrTgtDlt != reqMigrTgtDlt) {
            startMigrationCb(asyncHdr, migrationMsg->DLT_version, ERR_SM_TOK_MIGRATION_INPROGRESS);
        }
        return;
    }
*/
    // first disable GC and Tier Migration
    // Note that after disabling GC, GC work in QoS queue will still
    // finish... however, there is no correctness issue if we are running
    // GC job along with token migration, we just want to reduce the amount
    // of system work we do at the same time (for now, we should be able
    // to make it work running fully in parallel in the future).
    SmScavengerActionCmd scavCmd(fpi::FDSP_SCAVENGER_DISABLE,
                                 SM_CMD_INITIATOR_TOKEN_MIGRATION);
    err = objStorMgr->objectStore->scavengerControlCmd(&scavCmd);
    if (!err.ok()) {
        LOGERROR << "Failed to disable Scavenger; failing token migration";
        startMigrationCb(asyncHdr, migrationMsg->DLT_version, err);
        return;
    }

    SmTieringCmd tierCmd(SmTieringCmd::TIERING_DISABLE);
    err = objStorMgr->objectStore->tieringControlCmd(&tierCmd);
    if (!err.ok()) {
        LOGERROR << "Failed to disable Tier Migration; failing token migration";
        startMigrationCb(asyncHdr, migrationMsg->DLT_version, err);
        return;
    }

    // If SM is getting new DLT tokens that result in gaining new SM tokens
    // we need to open data and metadata stores for them for writing.
    SmTokenSet smTokens;
    for (auto migrGroup : migrationMsg->migrations) {
        // migrGroup is <source SM, set of DLT tokens> pair
        for (auto dltTok : migrGroup.tokens) {
            fds_token_id smTok = SmDiskMap::smTokenId(dltTok);
            smTokens.insert(smTok);
        }
    }
    err = objStorMgr->objectStore->openStore(smTokens);

    // start migration
    const DLT* dlt = objStorMgr->getDLT();

    if (dlt != NULL) {
        err = objStorMgr->migrationMgr->startMigration(migrationMsg,
                                                       std::bind(&SMSvcHandler::startMigrationCb,
                                                                 this,
                                                                 asyncHdr,
                                                                 migrationMsg->DLT_version,
                                                                 std::placeholders::_1),
                                                       objStorMgr->getUuid(),
                                                       dlt->getNumBitsForToken(),
                                                       SMMigrType::MIGR_SM_ADD_NODE,
                                                       false); //false because it's not a resync case
    } else {
        LOGERROR << "SM does not have any DLT; make sure that StartMigration is not "
                 << " called on addition of the first set of SMs to the domain";
        startMigrationCb(asyncHdr, migrationMsg->DLT_version, ERR_INVALID_DLT);
    }
}

void
SMSvcHandler::startMigrationCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                               fds_uint64_t dltVersion,
                               const Error& err)
{
    DBG(GLOGDEBUG << fds::logString(*asyncHdr));
    asyncHdr->msg_code = static_cast<int32_t>(err.GetErrno());

    fpi::CtrlNotifyMigrationStatusPtr msg(new fpi::CtrlNotifyMigrationStatus());
    msg->status.DLT_version = dltVersion;
    msg->status.context = 0;
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyMigrationStatus), *msg);
}

void
SMSvcHandler::migrationAbort(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                             CtrlNotifySMAbortMigrationPtr& abortMsg)
{
    Error err(ERR_OK);
    LOGNOTIFY << "Received Abort Migration, will revert to previously "
             << " commited DLT version " << abortMsg->DLT_version;

    auto abortMigrationReq = new SmIoAbortMigration(abortMsg);
    abortMigrationReq->io_type = FDS_SM_MIGRATION_ABORT;
    abortMigrationReq->abortMigrationDLTVersion = abortMsg->DLT_version;
    abortMigrationReq->targetDLTVersion = abortMsg->DLT_target_version;

    abortMigrationReq->abortMigrationCb = std::bind(&SMSvcHandler::migrationAbortCb,
                                                    this,
                                                    asyncHdr,
                                                    std::placeholders::_1,
                                                    std::placeholders::_2);

    err = objStorMgr->enqueueMsg(FdsSysTaskQueueId, abortMigrationReq);
    // TODO(Sean):
    // For now, assert that enqueueMsg() does not fail.  Need to think about what
    // error to reply to the OM.
    fds_verify(err.ok());
}

void
SMSvcHandler::migrationAbortCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                               const Error &err,
                               SmIoAbortMigration *abortMigrationReq)
{
    LOGMIGRATE << "SM Token Migration Abort CB on DLTVersion="
               << abortMigrationReq->abortMigrationDLTVersion;

    // send response
    fpi::CtrlNotifySMAbortMigrationPtr msg(new fpi::CtrlNotifySMAbortMigration());
    msg->DLT_version = abortMigrationReq->abortMigrationDLTVersion;
    asyncHdr->msg_code = static_cast<int32_t>(err.GetErrno());
    sendAsyncResp(*asyncHdr,
                  FDSP_MSG_TYPEID(fpi::CtrlNotifySMAbortMigration),
                  *(abortMigrationReq->abortMigrationReqMsg));
    /**
     * Enable scavenger and tiering, which was disabled before
     * starting the migration.
     * Eventually we should just remove this inter-dependency
     * between migration and gc.
     *
     * (Gurpreet): Revisit this fix. Make complete fix keeping in
     * mind combinations of
     * (
     *  Add node migration,
     *  Resync Migration,
     *  SM being a source,
     *  SM being a destination of a migration.
     * )
     */
    SmScavengerActionCmd scavCmd(fpi::FDSP_SCAVENGER_ENABLE,
                                 SM_CMD_INITIATOR_TOKEN_MIGRATION);
    objStorMgr->objectStore->scavengerControlCmd(&scavCmd);
    SmTieringCmd tierCmd(SmTieringCmd::TIERING_ENABLE);
    objStorMgr->objectStore->tieringControlCmd(&tierCmd);
}

/**
 * This is the message from destination SM (SM that asks this SM to migrate objects)
 * with an filter set of object metadata
 */
void
SMSvcHandler::initiateObjectSync(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                 fpi::CtrlObjectRebalanceFilterSetPtr& filterObjSet)
{
    Error err(ERR_OK);
    bool fault_enabled = false;
    LOGNORMAL << "Migration source request for token: " << filterObjSet->tokenId
              << " executor Id: " << std::hex << filterObjSet->executorID
              << " migration dest: " << asyncHdr->msg_src_uuid.svc_uuid
              << std::dec << " target dlt version: " << filterObjSet->targetDltVersion;

    // first disable GC and Tier Migration. If this SM is also a destination and
    // we already disabled GC and Tier Migration, disabling them again is a noop
    // Note that after disabling GC, GC work in QoS queue will still
    // finish... however, there is no correctness issue if we are running
    // GC job along with token migration, we just want to reduce the amount
    // of system work we do at the same time (for now, we should be able
    // to make it work running fully in parallel in the future).
    SmScavengerActionCmd scavCmd(fpi::FDSP_SCAVENGER_DISABLE,
                                 SM_CMD_INITIATOR_TOKEN_MIGRATION);
    err = objStorMgr->objectStore->scavengerControlCmd(&scavCmd);
    SmTieringCmd tierCmd(SmTieringCmd::TIERING_DISABLE);
    err = objStorMgr->objectStore->tieringControlCmd(&tierCmd);

    // tell migration mgr to start object rebalance
    const DLT* dlt = MODULEPROVIDER()->getSvcMgr()->getDltManager()->getDLT();

    fiu_do_on("resend.dlt.token.filter.set", fault_enabled = true;\
              LOGNOTIFY << "resend.dlt.token.filter.set fault point enabled";);
    if (objStorMgr->objectStore->isUnavailable()) {
        // object store failed to validate superblock or pass initial
        // integrity check
        err = ERR_NODE_NOT_ACTIVE;
        LOGCRITICAL << "SM service is unavailable " << std::hex
                    << objStorMgr->getUuid() << std::dec;
    } else if (fault_enabled || !(objStorMgr->objectStore->isReady())) {
        err = ERR_SM_NOT_READY_AS_MIGR_SRC;
        LOGDEBUG << "SM not ready as Migration source " << std::hex
                 << objStorMgr->getUuid() << std::dec
                 << " for token: " << filterObjSet->tokenId << std::hex
                 << " executor: " << filterObjSet->executorID;
        fiu_disable("resend.dlt.token.filter.set");
    } else {
        fds_verify(dlt != NULL);
        err = objStorMgr->migrationMgr->startObjectRebalance(filterObjSet,
                                                             asyncHdr->msg_src_uuid,
                                                             objStorMgr->getUuid(),
                                                             dlt->getNumBitsForToken(), dlt);
    }

    // respond with error code
    asyncHdr->msg_code = err.GetErrno();
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::EmptyMsg), fpi::EmptyMsg());
}

void
SMSvcHandler::syncObjectSet(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                            fpi::CtrlObjectRebalanceDeltaSetPtr& deltaObjSet)
{
    Error err(ERR_OK);
    LOGDEBUG << "Received Sync Object Set from SM " << std::hex
             << asyncHdr->msg_src_uuid.svc_uuid << std::dec;
    err = objStorMgr->migrationMgr->recvRebalanceDeltaSet(deltaObjSet);

    // TODO(Anna) respond with error, are we responding on success?
    fds_verify(err.ok() || (err == ERR_SM_TOK_MIGRATION_ABORTED));
}

void
SMSvcHandler::getMoreDelta(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                           fpi::CtrlGetSecondRebalanceDeltaSetPtr& getDeltaSetMsg)
{
    Error err(ERR_OK);
    LOGDEBUG << "Received get second delta set from destination SM "
             << std::hex << asyncHdr->msg_src_uuid.svc_uuid << std::dec
             << " executor ID " << getDeltaSetMsg->executorID;

    // notify migration mgr -- this call is sync
    err = objStorMgr->migrationMgr->startSecondObjectRebalance(getDeltaSetMsg,
                                                               asyncHdr->msg_src_uuid);

    // send response
    fpi::CtrlGetSecondRebalanceDeltaSetRspPtr msg(new fpi::CtrlGetSecondRebalanceDeltaSetRsp());
    asyncHdr->msg_code = static_cast<int32_t>(err.GetErrno());
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::CtrlGetSecondRebalanceDeltaSetRsp), *msg);
}

void
SMSvcHandler::finishClientTokenResync(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                      fpi::CtrlFinishClientTokenResyncMsgPtr& finishClientResyncMsg)
{
    Error err(ERR_OK);
    LOGDEBUG << "Received finish client resync msg from destination SM "
             << std::hex << asyncHdr->msg_src_uuid.svc_uuid << std::dec
             << " executor ID " << finishClientResyncMsg->executorID;

    // notify migration mgr -- this call is sync
    err = objStorMgr->migrationMgr->finishClientResync(finishClientResyncMsg->executorID);

    // send response
    fpi::CtrlFinishClientTokenResyncRspMsgPtr msg(new fpi::CtrlFinishClientTokenResyncRspMsg());
    asyncHdr->msg_code = static_cast<int32_t>(err.GetErrno());
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::CtrlFinishClientTokenResyncRspMsg), *msg);
}

void SMSvcHandler::shutdownSM(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                              boost::shared_ptr<fpi::PrepareForShutdownMsg>& shutdownMsg)
{
    LOGDEBUG << "Received shutdown message... shutting down...";
    if (!objStorMgr->isShuttingDown()) {
        objStorMgr->mod_disable_service();
        objStorMgr->mod_shutdown();
    }

    // respond to OM
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::EmptyMsg), fpi::EmptyMsg());
}

void SMSvcHandler::queryScrubberStatus(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                                       fpi::CtrlQueryScrubberStatusPtr &scrub_msg)
{
    GLOGDEBUG << "Scrubber status called";
    Error err(ERR_OK);
    fpi::CtrlQueryScrubberStatusRespPtr resp(new fpi::CtrlQueryScrubberStatusResp());
    SmScrubberGetStatusCmd scrubCmd(resp);
    err = objStorMgr->objectStore->scavengerControlCmd(&scrubCmd);

    hdr->msg_code = static_cast<int32_t>(err.GetErrno());
    GLOGDEBUG << "Scrubber status = " << resp->scrubber_status << " " << err;
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlQueryScrubberStatusResp), *resp);
}

void SMSvcHandler::setScrubberStatus(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                                     fpi::CtrlSetScrubberStatusPtr &scrub_msg)
{
    Error err(ERR_OK);
    fpi::CtrlSetScrubberStatusRespPtr resp(new fpi::CtrlSetScrubberStatusResp());
    LOGNORMAL << " receive scrubber cmd " << scrub_msg->scrubber_status;
    SmScrubberActionCmd scrubCmd(scrub_msg->scrubber_status);
    err = objStorMgr->objectStore->scavengerControlCmd(&scrubCmd);
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlSetScrubberStatusResp), *resp);
}

void SMSvcHandler::queryScavengerProgress(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                                          fpi::CtrlQueryScavengerProgressPtr &query_msg)
{
    Error err(ERR_OK);
    fpi::CtrlQueryScavengerProgressRespPtr resp(new fpi::CtrlQueryScavengerProgressResp());

    SmScavengerGetProgressCmd scavCmd;
    err = objStorMgr->objectStore->scavengerControlCmd(&scavCmd);
    resp->progress_pct = scavCmd.retProgress;
    hdr->msg_code = static_cast<int32_t>(err.GetErrno());
    GLOGDEBUG << "Response set: " << resp->progress_pct << " " << err;
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlQueryScavengerProgressResp), *resp);
}

void SMSvcHandler::setScavengerPolicy(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                                      fpi::CtrlSetScavengerPolicyPtr &policy_msg)
{
    fpi::CtrlSetScavengerPolicyRespPtr resp(new fpi::CtrlSetScavengerPolicyResp());
    Error err(ERR_OK);
    SmScavengerSetPolicyCmd scavCmd(policy_msg);
    err = objStorMgr->objectStore->scavengerControlCmd(&scavCmd);

    hdr->msg_code = static_cast<int32_t>(err.GetErrno());
    GLOGDEBUG << "Setting scavenger policy " << err;
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlSetScavengerPolicyResp), *resp);
}

void SMSvcHandler::queryScavengerPolicy(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                                        fpi::CtrlQueryScavengerPolicyPtr &query_msg)
{
    fpi::CtrlQueryScavengerPolicyRespPtr resp(new fpi::CtrlQueryScavengerPolicyResp());
    SmScavengerGetPolicyCmd scavCmd(resp);
    Error err = objStorMgr->objectStore->scavengerControlCmd(&scavCmd);

    hdr->msg_code = static_cast<int32_t>(err.GetErrno());
    GLOGDEBUG << "Got scavenger policy " << resp->dsk_threshold1;
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlQueryScavengerPolicyResp), *resp);
}

void SMSvcHandler::queryScavengerStatus(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                                        fpi::CtrlQueryScavengerStatusPtr &query_msg)
{
    Error err(ERR_OK);
    fpi::CtrlQueryScavengerStatusRespPtr resp(new fpi::CtrlQueryScavengerStatusResp());
    SmScavengerGetStatusCmd scavCmd(resp);
    err = objStorMgr->objectStore->scavengerControlCmd(&scavCmd);

    hdr->msg_code = static_cast<int32_t>(err.GetErrno());
    GLOGDEBUG << "Scavenger status = " << resp->status << " " << err;
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlQueryScavengerStatusResp), *resp);
}

void SMSvcHandler::getObject(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                             boost::shared_ptr<fpi::GetObjectMsg>& getObjMsg)  // NOLINT
{
    DBG(GLOGDEBUG << logString(*asyncHdr) << fds::logString(*getObjMsg));

    fiu_do_on("svc.drop.getobject", return);
#if 0
    fiu_do_on("svc.uturn.getobject",
              auto getReq = new SmIoGetObjectReq(getObjMsg); \
              boost::shared_ptr<GetObjectResp> resp = boost::make_shared<GetObjectResp>(); \
              getReq->getObjectNetResp = resp; \
              getObjectCb(asyncHdr, ERR_OK, getReq); return;);
#endif
#if 0
    fiu_do_on("svc.uturn.getobject", \
              mockHandler->schedule(mockTimeoutUs, \
                                    std::bind(&MockSMCallbacks::mockGetCb, this,\
                                              asyncHdr)); \
              return;);
#endif

    Error err(ERR_OK);
    ObjectID objId(getObjMsg->data_obj_id.digest);

    auto getReq = new SmIoGetObjectReq(getObjMsg);
    getReq->io_type = FDS_SM_GET_OBJECT;
    fds_volid_t volId(getObjMsg->volume_id);
    getReq->setVolId(volId);
    getReq->setObjId(objId);
    getReq->obj_data.obj_id = getObjMsg->data_obj_id;
    // perf-trace related data
    getReq->opReqFailedPerfEventType = PerfEventType::SM_GET_OBJ_REQ_ERR;
    getReq->opReqLatencyCtx.type = PerfEventType::SM_E2E_GET_OBJ_REQ;
    getReq->opReqLatencyCtx.reset_volid(volId);
    getReq->opLatencyCtx.type = PerfEventType::SM_GET_IO;
    getReq->opLatencyCtx.reset_volid(volId);
    getReq->opQoSWaitCtx.type = PerfEventType::SM_GET_QOS_QUEUE_WAIT;
    getReq->opQoSWaitCtx.reset_volid(volId);

    // Set the client's ID to use to serialization
    getReq->setClientSvcId(asyncHdr->msg_src_uuid);

    getReq->response_cb = std::bind(&SMSvcHandler::getObjectCb,
                                    this,
                                    asyncHdr,
                                    std::placeholders::_1,
                                    std::placeholders::_2);

    // start measuring E2E latency
    PerfTracer::tracePointBegin(getReq->opReqLatencyCtx);

    // check if DLT token ready -- we are doing it after creating
    // get request, because callback needs it
    if (!objStorMgr->migrationMgr->isDltTokenReady(objId)) {
        LOGDEBUG << "DLT token not ready, not going to do GET for " << objId;
        getObjectCb(asyncHdr, ERR_TOKEN_NOT_READY, getReq);
        return;
    }

    err = objStorMgr->enqueueMsg(getReq->getVolId(), getReq);
    if (err != fds::ERR_OK) {
        LOGERROR << "Failed to enqueue to SmIoGetObjectReq to StorMgr.  Error: "
                 << err;
        getObjectCb(asyncHdr, err, getReq);
    }
}

void SMSvcHandler::getObjectCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                               const Error &err,
                               SmIoGetObjectReq *getReq)
{
    DBG(GLOGDEBUG << fds::logString(*asyncHdr));

    boost::shared_ptr<GetObjectResp> resp;
    asyncHdr->msg_code = static_cast<int32_t>(err.GetErrno());
    resp = getReq->getObjectNetResp;

    // E2E latency end
    PerfTracer::tracePointEnd(getReq->opReqLatencyCtx);
    if (!err.ok()) {
        PerfTracer::incr(getReq->opReqFailedPerfEventType,
                         getReq->getVolId());
    }

    // Independent if error happend, check if this request matches
    // currently closed DLT version. If not, we cannot guarantee that
    // successessful get was in fact successful.
    //
    // Requests to SM are relative to a DLT table that maps which
    // SMs are responsible for which objects. If the DLT version
    // being used by the sender is stale then this may not be the
    // correct SM to contact. We know if other version are stale if
    // our current version has been closed (see OM table propagation
    // protocol). If we don't have a DLT version yet then have SM process
    // the request.
    const DLT *curDlt = objStorMgr->getDLT();
    if ((curDlt) &&
        (curDlt->isClosed()) &&
        (curDlt->getVersion() > (fds_uint64_t)asyncHdr->dlt_version)) {
        // Tell the sender that their DLT version is invalid.
        LOGDEBUG << "Returning DLT mismatch using version "
                 << (fds_uint64_t)asyncHdr->dlt_version << " with current closed version "
                 << curDlt->getVersion();

        asyncHdr->msg_code = ERR_IO_DLT_MISMATCH;
    }

    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::GetObjectResp), *resp);
    delete getReq;
}

void SMSvcHandler::putObject(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                             boost::shared_ptr<fpi::PutObjectMsg>& putObjMsg)  // NOLINT
{
    Error err(ERR_OK);
    fds_volid_t volId(putObjMsg->volume_id);
    ObjectID objId(putObjMsg->data_obj_id.digest);

    if ((objStorMgr->testUturnAll == true) ||
        (objStorMgr->testUturnPutObj == true)) {
        LOGDEBUG << "Uturn testing put object "
                 << fds::logString(*asyncHdr) << fds::logString(*putObjMsg);
        putObjectCb(asyncHdr, ERR_OK, NULL);
        return;
    }

    if (!(objStorMgr->getVol(volId))) {
        putObjectCb(asyncHdr, ERR_VOL_NOT_FOUND, NULL);
    }

    DBG(GLOGDEBUG << fds::logString(*asyncHdr) << fds::logString(*putObjMsg));

    fiu_do_on("svc.drop.putobject", return);
    // fiu_do_on("svc.uturn.putobject", putObjectCb(asyncHdr, ERR_OK, NULL); return;);
#if 0
    fiu_do_on("svc.uturn.putobject", \
              mockHandler->schedule(mockTimeoutUs, \
                                    std::bind(&MockSMCallbacks::mockPutCb, this,\
                                              asyncHdr)); \
              return;);
#endif

    auto putReq = new SmIoPutObjectReq(putObjMsg);
    putReq->io_type = FDS_SM_PUT_OBJECT;
    putReq->setVolId(volId);
    putReq->dltVersion = asyncHdr->dlt_version;
    putReq->forwardedReq = putObjMsg->forwardedReq;
    putReq->setObjId(objId);

    // Set the client's ID to use to serialization
    putReq->setClientSvcId(asyncHdr->msg_src_uuid);

    // perf-trace related data
    putReq->opReqFailedPerfEventType = PerfEventType::SM_PUT_OBJ_REQ_ERR;
    putReq->opReqLatencyCtx.type = PerfEventType::SM_E2E_PUT_OBJ_REQ;
    putReq->opReqLatencyCtx.reset_volid(volId);
    putReq->opLatencyCtx.type = PerfEventType::SM_PUT_IO;
    putReq->opLatencyCtx.reset_volid(volId);
    putReq->opQoSWaitCtx.type = PerfEventType::SM_PUT_QOS_QUEUE_WAIT;
    putReq->opQoSWaitCtx.reset_volid(volId);

    putReq->response_cb= std::bind(&SMSvcHandler::putObjectCb,
                                   this,
                                   asyncHdr,
                                   std::placeholders::_1,
                                   std::placeholders::_2);

    // start measuring E2E latency
    PerfTracer::tracePointBegin(putReq->opReqLatencyCtx);

    // check if DLT token ready -- we are doing it after creating
    // put request, because callback needs it
    if (!putObjMsg->forwardedReq && !objStorMgr->migrationMgr->isDltTokenReady(objId)) {
        LOGDEBUG << "DLT token not ready, not going to do PUT for " << objId;
        putObjectCb(asyncHdr, ERR_TOKEN_NOT_READY, putReq);
        return;
    }

    err = objStorMgr->enqueueMsg(putReq->getVolId(), putReq);
    if (err != fds::ERR_OK) {
    	if (err != fds::ERR_VOL_NOT_FOUND) {
    		/**
    		 * Race cond: SM may not have the vol descriptors
    		 * ready yet even though it's finished pulling the DLT.
    		 */
    		fds_assert(!"Hit an error in enqueing");
    	} else {
            /**
             * TODO(neil): This needs to be fixed. See FS-2229
             */
            LOGWARN << "Hit FS-2229. This needs to be fixed.";
        }
        LOGERROR << "Failed to enqueue to SmIoPutObjectReq to StorMgr.  Error: "
                 << err;
        putObjectCb(asyncHdr, err, putReq);
    }
}

#if 0
void SMSvcHandler::mockPutCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr)
{
    auto resp = boost::make_shared<fpi::PutObjectRspMsg>();
    asyncHdr->msg_code = ERR_OK;
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::PutObjectRspMsg), *resp);
}

void SMSvcHandler::mockGetCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr)
{
    auto resp = boost::make_shared<fpi::GetObjectResp>();
    resp->data_obj = std::move(std::string(4096, 'a'));
    asyncHdr->msg_code = ERR_OK;
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::GetObjectResp), *resp);
}
#endif

void SMSvcHandler::putObjectCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                               const Error &err,
                               SmIoPutObjectReq* putReq)
{
    DBG(GLOGDEBUG << fds::logString(*asyncHdr));

    // E2E latency end
    if (putReq) {
        // check if there was not a uturn test
        PerfTracer::tracePointEnd(putReq->opReqLatencyCtx);
        if (!err.ok()) {
            PerfTracer::incr(putReq->opReqFailedPerfEventType,
                             putReq->getVolId());
        }
    }

    auto resp = boost::make_shared<fpi::PutObjectRspMsg>();
    asyncHdr->msg_code = static_cast<int32_t>(err.GetErrno());

    // Independent if error happend, check if this request matches
    // currently closed DLT version. This should not happen, but if
    // it did, we may end up with incorrect object refcount.
    // TODO(Anna) If we see this error happening, we will have to
    // process DLT close by taking write lock on object store
    // (i.e. object store must not process any IO)
    //
    // Requests to SM are relative to a DLT table that maps which
    // SMs are responsible for which objects. If the DLT version
    // being used by the sender is stale then this may not be the
    // correct SM to contact. We know if other version are stale if
    // our current version has been closed (see OM table propagation
    // protocol). If we don't have a DLT version yet then have SM process
    // the request.
    const DLT *curDlt = objStorMgr->getDLT();
    if ((curDlt) &&
        (curDlt->isClosed()) &&
        (curDlt->getVersion() > (fds_uint64_t)asyncHdr->dlt_version)) {
        // Tell the sender that their DLT version is invalid.
        LOGCRITICAL << "Returning DLT mismatch using version "
                    << (fds_uint64_t)asyncHdr->dlt_version
                    << " with current closed version "
                    << curDlt->getVersion();

        asyncHdr->msg_code = ERR_IO_DLT_MISMATCH;
    }

    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::PutObjectRspMsg), *resp);

    if ((objStorMgr->testUturnAll == true) ||
        (objStorMgr->testUturnPutObj == true)) {
        fds_verify(putReq == NULL);
        return;
    }

    delete putReq;
}

void SMSvcHandler::deleteObject(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                boost::shared_ptr<fpi::DeleteObjectMsg>& deleteObjMsg)
{
    if (objStorMgr->testUturnAll == true) {
        LOGDEBUG << "Uturn testing delete object "
                 << fds::logString(*asyncHdr) << fds::logString(*deleteObjMsg);
        deleteObjectCb(asyncHdr, ERR_OK, NULL);
        return;
    }

    DBG(GLOGDEBUG << fds::logString(*asyncHdr) << fds::logString(*deleteObjMsg));
    Error err(ERR_OK);
    ObjectID objId(deleteObjMsg->objId.digest);
    auto delReq = new SmIoDeleteObjectReq(deleteObjMsg);
    delReq->setVolId((fds_volid_t)(deleteObjMsg->volId));
    delReq->setObjId(ObjectID(deleteObjMsg->objId.digest));

    // Set the client's ID to use to serialization
    delReq->setClientSvcId(asyncHdr->msg_src_uuid);

    // Set delReq stuffs
    delReq->io_type = FDS_SM_DELETE_OBJECT;

    delReq->setVolId(fds_volid_t(deleteObjMsg->volId));
    delReq->dltVersion = asyncHdr->dlt_version;
    delReq->setObjId(objId);
    delReq->forwardedReq = deleteObjMsg->forwardedReq;

    // perf-trace related data
    delReq->opReqFailedPerfEventType = PerfEventType::SM_DELETE_OBJ_REQ_ERR;
    delReq->opReqLatencyCtx.type = PerfEventType::SM_E2E_DELETE_OBJ_REQ;
    delReq->opLatencyCtx.type = PerfEventType::SM_DELETE_IO;
    delReq->opQoSWaitCtx.type = PerfEventType::SM_DELETE_QOS_QUEUE_WAIT;

    delReq->response_cb = std::bind(&SMSvcHandler::deleteObjectCb,
                                    this,
                                    asyncHdr,
                                    std::placeholders::_1,
                                    std::placeholders::_2);

    // start measuring E2E latency
    PerfTracer::tracePointBegin(delReq->opReqLatencyCtx);

    // check if DLT token ready -- we are doing it after creating
    // delete request, because callback needs it
    if (!deleteObjMsg->forwardedReq && !objStorMgr->migrationMgr->isDltTokenReady(objId)) {
        LOGDEBUG << "DLT token not ready, not going to do DELETE for " << objId;
        deleteObjectCb(asyncHdr, ERR_TOKEN_NOT_READY, delReq);
        return;
    }

    // DM sending the Ack to OM  for delete request can not  be timed correctly. we observed that 
    // SM volume  queues are deleted  as part of the volume delete before expunge is complete. 
    // hence moved the delete queue to system queue. we will hav to  revisit this

    err = objStorMgr->enqueueMsg(delReq->getVolId(), delReq);
    if (err == ERR_NOT_FOUND) {
       err = objStorMgr->enqueueMsg(FdsSysTaskQueueId, delReq);
    }
    if (err != fds::ERR_OK) {
        LOGERROR << "Failed to enqueue to SmIoDeleteObjectReq to StorMgr.  Error: "
                 << err;
        deleteObjectCb(asyncHdr, err, delReq);
    }
}

void SMSvcHandler::deleteObjectCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                  const Error &err,
                                  SmIoDeleteObjectReq* del_req)
{
    DBG(GLOGDEBUG << fds::logString(*asyncHdr));

    // E2E latency end
    if (del_req) {
        PerfTracer::tracePointEnd(del_req->opReqLatencyCtx);
        if (!err.ok()) {
            PerfTracer::incr(del_req->opReqFailedPerfEventType,
                             del_req->getVolId());
        }
    }

    auto resp = boost::make_shared<fpi::DeleteObjectRspMsg>();
    asyncHdr->msg_code = static_cast<int32_t>(err.GetErrno());

    // Independent if error happend, check if this request matches
    // currently closed DLT version. This should not happen, but if
    // it did, we may end up with incorrect object refcount.
    // TODO(Anna) If we see this error happening, we will have to
    // process DLT close by taking write lock on object store
    // (i.e. object store must not process any IO)
    //
    // Requests to SM are relative to a DLT table that maps which
    // SMs are responsible for which objects. If the DLT version
    // being used by the sender is stale then this may not be the
    // correct SM to contact. We know if other version are stale if
    // our current version has been closed (see OM table propagation
    // protocol). If we don't have a DLT version yet then have SM process
    // the request.
    const DLT *curDlt = objStorMgr->getDLT();
    if ((curDlt) &&
        (curDlt->isClosed()) &&
        (curDlt->getVersion() > (fds_uint64_t)asyncHdr->dlt_version)) {
        // Tell the sender that their DLT version is invalid.
        LOGCRITICAL << "Returning DLT mismatch using version "
                    << (fds_uint64_t)asyncHdr->dlt_version
                    << " with current closed version "
                    << curDlt->getVersion();

        asyncHdr->msg_code = ERR_IO_DLT_MISMATCH;
    }

    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::DeleteObjectRspMsg), *resp);

    if (del_req) {
        delete del_req;
    }
}
// NotifySvcChange
// ---------------
//
void
SMSvcHandler::notifySvcChange(boost::shared_ptr<fpi::AsyncHdr>    &hdr,
                              boost::shared_ptr<fpi::NodeSvcInfo> &msg)
{
}

// NotifyAddVol
// ------------
//
void
SMSvcHandler::NotifyAddVol(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                           boost::shared_ptr<fpi::CtrlNotifyVolAdd> &vol_msg)
{
    fds_volid_t volumeId(vol_msg->vol_desc.volUUID);
    VolumeDesc vdb(vol_msg->vol_desc);
    FDSP_NotifyVolFlag vol_flag = vol_msg->vol_flag;
    GLOGNOTIFY << "Received create for vol "
               << "[" << std::hex << volumeId << std::dec << ", "
               << vdb.getName() << "]";

    Error err = objStorMgr->regVol(vdb);
    if (err.ok()) {
        StorMgrVolume * vol = objStorMgr->getVol(volumeId);
        fds_assert(vol != NULL);

        fds_volid_t queueId = vol->getQueue()->getVolUuid();
        if (!objStorMgr->getQueue(queueId)) {
            err = objStorMgr->regVolQos(queueId, static_cast<FDS_VolumeQueue*>(
                vol->getQueue().get()));
        }

        if (!err.ok()) {
            // most likely axceeded min iops
            objStorMgr->deregVol(volumeId);
        }
    }
    if (!err.ok()) {
        GLOGERROR << "Registration failed for vol id " << std::hex << volumeId
                  << std::dec << " " << err;
    }
    hdr->msg_code = err.GetErrno();
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyVolAdd), *vol_msg);

    // Start the hybrid tier migration on the first volume add call. The volume information
    // is propagated after DLT update, and this can cause missing volume and associated volume
    // policy.
    // Although this is called every time when a volume is added, but hybrid tier migration
    // controller will decided if it need to start tier migration or not.
    SmTieringCmd tierCmd(SmTieringCmd::TIERING_START_HYBRIDCTRLR_AUTO);
    err = objStorMgr->objectStore->tieringControlCmd(&tierCmd);
    if (err != ERR_OK) {
        LOGWARN << "Failed to enable Hybrid Tier Migration";
    }
}

// NotifyRmVol
// -----------
//
void
SMSvcHandler::NotifyRmVol(boost::shared_ptr<fpi::AsyncHdr>            &hdr,
                          boost::shared_ptr<fpi::CtrlNotifyVolRemove> &vol_msg)
{
    fds_volid_t volumeId(vol_msg->vol_desc.volUUID);
    std::string volName  = vol_msg->vol_desc.vol_name;

    if (vol_msg->vol_flag == FDSP_NOTIFY_VOL_CHECK_ONLY) {
        GLOGNOTIFY << "Received del chk for vol "
                   << "[" << volumeId
                   <<  ":" << volName << "]";
    } else  {
        GLOGNOTIFY << "Received delete for vol "
                   << "[" << std::hex << volumeId << std::dec << ", "
                   << volName << "]";

        // wait for queue to be empty.
        fds_uint32_t qSize = objStorMgr->qosCtrl->queueSize(volumeId);
        int count = 0;
        while (qSize > 0) {
            LOGWARN << "wait for delete - q not empty for vol:" << volumeId
                    << " size:" << qSize
                    << " waited for [" << ++count << "] secs";
            usleep(1*1000*1000);
            qSize = objStorMgr->qosCtrl->queueSize(volumeId);
        }

        objStorMgr->quieseceIOsQos(volumeId);
        objStorMgr->deregVolQos(volumeId);
        objStorMgr->deregVol(volumeId);
        objStorMgr->objectStore->removeObjectSet(volumeId);
    }
    hdr->msg_code = 0;
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyVolRemove), *vol_msg);
}

// NotifyModVol
// ------------
//
void
SMSvcHandler::NotifyModVol(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                           boost::shared_ptr<fpi::CtrlNotifyVolMod> &vol_msg)
{
    Error err;
    VolumeDesc vdbc(vol_msg->vol_desc), * vdb = &vdbc;
    fds_volid_t volumeId(vol_msg->vol_desc.volUUID);
    GLOGNOTIFY << "Received modify for vol "
               << "[" << std::hex << volumeId << std::dec << ", "
               << vdb->getName() << "]";

    StorMgrVolume * vol = objStorMgr->getVol(volumeId);
    fds_assert(vol != NULL);
    if (vol->voldesc->mediaPolicy != vdb->mediaPolicy) {
        // TODO(Rao): Total hack. This should go through some sort of synchronization.
        // I can't fina a better interface for doing this in the existing code
        vol->voldesc->mediaPolicy = vdb->mediaPolicy;
    }

    vol->voldesc->modifyPolicyInfo(vdb->iops_assured, vdb->iops_throttle, vdb->relativePrio);
    err = objStorMgr->modVolQos(vol->getVolId(),
                                vdb->iops_assured, vdb->iops_throttle, vdb->relativePrio);
    if ( !err.ok() )  {
        GLOGERROR << "Modify volume policy failed for vol " << vdb->getName()
                  << std::hex << volumeId << std::dec << " error: "
                  << err.GetErrstr();
    }
    hdr->msg_code = err.GetErrno();
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyVolMod), *vol_msg);
}

// CtrlStartHybridTierCtrlrMsg
//
// Manually start Hybrid Tiering.
// ----------------
//
void
SMSvcHandler::startHybridTierCtrlr(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                                   boost::shared_ptr<fpi::CtrlStartHybridTierCtrlrMsg> &hbtMsg)
{
    SmTieringCmd tierCmd(SmTieringCmd::TIERING_START_HYBRIDCTRLR_MANUAL);
    Error err = objStorMgr->objectStore->tieringControlCmd(&tierCmd);
    if (err != ERR_OK) {
        LOGWARN << err;
    }
}

void SMSvcHandler::addObjectRef(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                boost::shared_ptr<fpi::AddObjectRefMsg>& addObjRefMsg)
{
    DBG(GLOGDEBUG << fds::logString(*asyncHdr) << " " << fds::logString(*addObjRefMsg));
    Error err(ERR_OK);

    auto addObjRefReq = new SmIoAddObjRefReq(addObjRefMsg);

    addObjRefReq->dltVersion = asyncHdr->dlt_version;
    addObjRefReq->forwardedReq = addObjRefMsg->forwardedReq;

    // Set the client's ID to use to serialization
    addObjRefReq->setClientSvcId(asyncHdr->msg_src_uuid);

    // perf-trace related data
    addObjRefReq->opReqFailedPerfEventType = PerfEventType::SM_ADD_OBJ_REF_REQ_ERR;
    addObjRefReq->opReqLatencyCtx.type = PerfEventType::SM_E2E_ADD_OBJ_REF_REQ;
    addObjRefReq->opLatencyCtx.type = PerfEventType::SM_ADD_OBJ_REF_IO;
    addObjRefReq->opQoSWaitCtx.type = PerfEventType::SM_ADD_OBJ_REF_QOS_QUEUE_WAIT;

    addObjRefReq->response_cb = std::bind(&SMSvcHandler::addObjectRefCb,
                                          this,
                                          asyncHdr,
                                          std::placeholders::_1,
                                          std::placeholders::_2);

    // start measuring E2E latency
    PerfTracer::tracePointBegin(addObjRefReq->opReqLatencyCtx);

    err = objStorMgr->enqueueMsg(addObjRefReq->getSrcVolId(), addObjRefReq);
    if (err != fds::ERR_OK) {
        LOGERROR << "Failed to enqueue to SmIoAddObjRefReq to StorMgr.  Error: "
                 << err;
        addObjectRefCb(asyncHdr, err, addObjRefReq);
    }
}

void SMSvcHandler::addObjectRefCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                  const Error &err, SmIoAddObjRefReq* addObjRefReq)
{
    DBG(GLOGDEBUG << fds::logString(*asyncHdr));

    // E2E latency end
    PerfTracer::tracePointEnd(addObjRefReq->opReqLatencyCtx);
    if (!err.ok()) {
        PerfTracer::incr(addObjRefReq->opReqFailedPerfEventType,
                         addObjRefReq->getVolId());
    }

    auto resp = boost::make_shared<fpi::AddObjectRefRspMsg>();
    asyncHdr->msg_code = static_cast<int32_t>(err.GetErrno());

    // Independent of error happend, check if this request matches
    // currently closed DLT version. This should not happen, but if
    // it did, we may end up with incorrect object refcount.
    // TODO(Anna) If we see this error happening, we will have to
    // process DLT close by taking write lock on object store
    // (i.e. object store must not process any IO)
    //
    // Requests to SM are relative to a DLT table that maps which
    // SMs are responsible for which objects. If the DLT version
    // being used by the sender is stale then this may not be the
    // correct SM to contact. We know if other version are stale if
    // our current version has been closed (see OM table propagation
    // protocol). If we don't have a DLT version yet then have SM process
    // the request.
    const DLT *curDlt = objStorMgr->getDLT();
    if ((curDlt) &&
        (curDlt->isClosed()) &&
        (curDlt->getVersion() > (fds_uint64_t)asyncHdr->dlt_version)) {
        // Tell the sender that their DLT version is invalid.
        LOGCRITICAL << "Returning DLT mismatch using version "
                    << (fds_uint64_t)asyncHdr->dlt_version
                    << " with current closed version "
                    << curDlt->getVersion();

        asyncHdr->msg_code = ERR_IO_DLT_MISMATCH;
    }

    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::AddObjectRefRspMsg), *resp);

    delete addObjRefReq;
}

void
SMSvcHandler::NotifyScavenger(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                              boost::shared_ptr<fpi::CtrlNotifyScavenger> &msg)
{
    Error err(ERR_OK);
    LOGNORMAL << " receive scavenger cmd " << msg->scavenger.cmd;
    SmScavengerActionCmd scavCmd(msg->scavenger.cmd, SM_CMD_INITIATOR_USER);
    err = objStorMgr->objectStore->scavengerControlCmd(&scavCmd);

    hdr->msg_code = 0;
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyScavenger), *msg);
}

// NotifyDLTUpdate
// ---------------
//
void
SMSvcHandler::NotifyDLTUpdate(boost::shared_ptr<fpi::AsyncHdr>            &hdr,
                              boost::shared_ptr<fpi::CtrlNotifyDLTUpdate> &dlt)
{
    Error err(ERR_OK);
    LOGDEBUG << "Received new DLT commit version  "
             << dlt->dlt_version;
    err = MODULEPROVIDER()->getSvcMgr()->updateDlt(dlt->dlt_data.dlt_type,
                                                   dlt->dlt_data.dlt_data, nullptr);
    if (err.ok()) {
        err = objStorMgr->handleDltUpdate();
    } else if (err == ERR_DUPLICATE) {
        LOGWARN << "Received duplicate DLT, ignoring";
        err = ERR_OK;
    }

    LOGNOTIFY << "Sending DLT commit (version " << dlt->dlt_version
              << ") response to OM with result " << err;
    hdr->msg_code = err.GetErrno();
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::EmptyMsg), fpi::EmptyMsg());
}

// NotifyDLTClose
// ---------------
//
void
SMSvcHandler::NotifyDLTClose(boost::shared_ptr<fpi::AsyncHdr> &asyncHdr,
                             boost::shared_ptr<fpi::CtrlNotifyDLTClose> &dlt)
{
    Error err(ERR_OK);
    LOGNOTIFY << "Receiving DLT Close for DLT version " << (dlt->dlt_close).DLT_version;

    // OM should not be sending DLT close for DLT version to which this SM
    // did not get DLT update, but just make sure SM ignores DLT close
    // for DLT this SM does not know about
    const DLT *curDlt = objStorMgr->getDLT();
    if (curDlt == nullptr) {
        LOGNOTIFY << "SM did not receive DLT for version " << (dlt->dlt_close).DLT_version
                  << ", will ignore this DLT close";
        // OK to OM
        sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::EmptyMsg), fpi::EmptyMsg());
        return;
    } else if (curDlt->getVersion() != (fds_uint64_t)((dlt->dlt_close).DLT_version)) {
        LOGNOTIFY << "SM received DLT close for the version " << (dlt->dlt_close).DLT_version
                  << ", but the current DLT version is " << curDlt->getVersion()
                  << ". SM will ignore this DLT close";
        fds_verify((fds_uint64_t)((dlt->dlt_close).DLT_version) < curDlt->getVersion());
        // otherwise OK to OM
        sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::EmptyMsg), fpi::EmptyMsg());
        return;
    }

    // Set closed flag for the DLT. We use it for garbage collecting
    // DLT tokens that are no longer belong to this SM. We want to make
    // sure we garbage collect only when DLT is closed
    err = MODULEPROVIDER()->getSvcMgr()->getDltManager()->setCurrentDltClosed();
    if (err == ERR_NOT_FOUND) {
        LOGERROR << "SM received DLT close without receiving DLT, ok for now, but fix OM!!!";
        // returning OK to OM
        sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::EmptyMsg), fpi::EmptyMsg());
        return;
    }

    auto DLTCloseReq = new SmIoNotifyDLTClose(dlt);
    DLTCloseReq->setVolId(FdsSysTaskQueueId);
    DLTCloseReq->io_type = FDS_SM_NOTIFY_DLT_CLOSE;
    DLTCloseReq->closeDLTVersion = (dlt->dlt_close).DLT_version;

    DLTCloseReq->closeDLTCb = std::bind(&SMSvcHandler::NotifyDLTCloseCb,
                                              this,
                                              asyncHdr,
                                              std::placeholders::_1,
                                              std::placeholders::_2);
    err = objStorMgr->enqueueMsg(FdsSysTaskQueueId, DLTCloseReq);
    // TODO(Sean):
    // For now, assert that enqueueMsg() does not fail.  Need to think about what
    // error to reply to the OM.
    fds_verify(err.ok());
}

void
SMSvcHandler::NotifyDLTCloseCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                               const Error &err,
                               SmIoNotifyDLTClose *DLTCloseReq)

{
    LOGNOTIFY << "NotifyDLTCloseCB called with DLTversion="
              << DLTCloseReq->closeDLTVersion
              << " with error " << err;

    // send response
    asyncHdr->msg_code = err.GetErrno();
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::EmptyMsg), fpi::EmptyMsg());
}


// NotifyDMTUpdate
// Necessary for streaming stats
// ----------------
//
void
SMSvcHandler::NotifyDMTUpdate(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                              boost::shared_ptr<fpi::CtrlNotifyDMTUpdate> &dmt)
{
    Error err(ERR_OK);
    LOGNOTIFY << "Received new DMT commit version  "
                << dmt->dmt_data.dmt_type;
    err = MODULEPROVIDER()->getSvcMgr()->updateDmt(dmt->dmt_data.dmt_type,
                                                   dmt->dmt_data.dmt_data,
                                                   nullptr);
    if (err == ERR_DUPLICATE) {
        LOGWARN << "Received duplicate DMT, ignoring";
        err = ERR_OK;
    }
    hdr->msg_code = err.GetErrno();
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyDMTUpdate), *dmt);
}

void
SMSvcHandler::NotifySMCheck(boost::shared_ptr<fpi::AsyncHdr>& hdr,
                            boost::shared_ptr<fpi::CtrlNotifySMCheck>& msg)
{
    Error err(ERR_OK);

    LOGDEBUG << "Received SMCheck cmd=" << msg->SmCheckCmd << " with target tokens list of len "
             << msg->targetTokens.size();

    std::set<fds_token_id> tgtTokens;
    for (auto token : msg->targetTokens) {
        tgtTokens.insert(token);
    }
    SmCheckActionCmd actionCmd(msg->SmCheckCmd, tgtTokens);
    err = objStorMgr->objectStore->SmCheckControlCmd(&actionCmd);
    // (Phillip) NotifySMCheck should not have a response - should be fire and forget
    //hdr->msg_code = static_cast<int32_t>(err.GetErrno());
    //sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifySMCheck), *msg);
}

void
SMSvcHandler::querySMCheckStatus(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                                 boost::shared_ptr<fpi::CtrlNotifySMCheckStatus>& msg)
{
    Error err(ERR_OK);

    LOGDEBUG << "Received SMCheck status query";

    fpi::CtrlNotifySMCheckStatusRespPtr resp(new fpi::CtrlNotifySMCheckStatusResp());
    SmCheckStatusCmd statusCmd(resp);
    err = objStorMgr->objectStore->SmCheckControlCmd(&statusCmd);
    hdr->msg_code = static_cast<int32_t>(err.GetErrno());
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifySMCheckStatusResp), *resp);
}

void
SMSvcHandler::activeObjects(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                            boost::shared_ptr<fpi::ActiveObjectsMsg>& msg)
{
    Error err(ERR_OK);

    LOGDEBUG << hdr;

    /**
        msg->filename
        msg->checksum
        msg->volumeIds
        msg->token
    */
    std::string filename;
    // verify the file checksum
    if (msg->filename.empty() && msg->checksum.empty()) {
        LOGDEBUG << "no active objects for token:" << msg->token
                 << " for [" << msg->volumeIds.size() <<"]";
    } else {
        filename = objStorMgr->fileTransfer->getFullPath(msg->filename);
        if (!util::fileExists(filename)) {
            err = ERR_FILE_DOES_NOT_EXIST;
            LOGERROR << "active object file ["
                     << filename
                     << "] does not exist";
        } else {
            std::string chksum = util::getFileChecksum(filename);
            if (chksum != msg->checksum) {
                LOGERROR << "file checksum mismatch [orig:" << msg->checksum
                         << " new:" << chksum << "]"
                         << " file:" << filename;
                err = ERR_CHECKSUM_MISMATCH;
            }
        }
    }

    TimeStamp ts = msg->scantimestamp * 1000 * 1000 * 1000;
    for (auto volId : msg->volumeIds) {
        objStorMgr->objectStore->addObjectSet(msg->token, fds_volid_t(volId),
                                              ts, filename);
    }

    fpi::ActiveObjectsRespMsgPtr resp(new fpi::ActiveObjectsRespMsg());
    hdr->msg_code = static_cast<int32_t>(err.GetErrno());
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::ActiveObjectsRspMsg), *resp);
}

void SMSvcHandler::diskMapChange(ASYNC_HANDLER_PARAMS(NotifyDiskMapChange)) {
    LOGNOTIFY << "Received a disk-map change notification";
    objStorMgr->handleNewDiskMap();
}

void SMSvcHandler::genericCommand(ASYNC_HANDLER_PARAMS(GenericCommandMsg)) {
    if (msg->command == "refscan.done") {
        // start the scavenger if we have enough data.
        if (objStorMgr->objectStore->haveAllObjectSets()) {
            LOGNORMAL << "auto starting scavenger due to enough data to process";
            SmScavengerActionCmd scavCmd(fpi::FDSP_SCAVENGER_START, SM_CMD_INITIATOR_NOT_SET);
            Error err = objStorMgr->objectStore->scavengerControlCmd(&scavCmd);
        }
    } else {
        LOGCRITICAL << "unexpected command received : " << msg;
    }
}

}  // namespace fds
