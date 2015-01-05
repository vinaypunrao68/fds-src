/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <StorMgr.h>
#include <net/net-service-tmpl.hpp>
#include <fdsp_utils.h>
#include <fds_assert.h>
#include <SMSvcHandler.h>
// #include <platform/flags_map.h>
#include <sm-platform.h>
#include <string>
#include <net/SvcRequest.h>
#include <fiu-local.h>
#include <random>
#include <chrono>
#include <MockSMCallbacks.h>
#include <net/MockSvcHandler.h>
#include <fds_timestamp.h>
#include <OMgrClient.h>

namespace fds {

extern ObjectStorMgr    *objStorMgr;

SMSvcHandler::SMSvcHandler()
{
    mockTimeoutEnabled = gModuleProvider->get_fds_config()->\
                         get<bool>("fds.sm.testing.enable_mocking");
    mockTimeoutUs = gModuleProvider->get_fds_config()->\
                    get<uint32_t>("fds.sm.testing.mocktimeout");
    if (true == mockTimeoutEnabled) {
        mockHandler.reset(new MockSvcHandler());
    }

    REGISTER_FDSP_MSG_HANDLER(fpi::GetObjectMsg, getObject);
    REGISTER_FDSP_MSG_HANDLER(fpi::PutObjectMsg, putObject);
    REGISTER_FDSP_MSG_HANDLER(fpi::DeleteObjectMsg, deleteObject);

    REGISTER_FDSP_MSG_HANDLER(fpi::NodeSvcInfo, notifySvcChange);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyVolAdd, NotifyAddVol);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyVolRemove, NotifyRmVol);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyVolMod, NotifyModVol);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyScavenger, NotifyScavenger);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlQueryScavengerStatus, queryScavengerStatus);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlQueryScavengerProgress, queryScavengerProgress);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlSetScavengerPolicy, setScavengerPolicy);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlQueryScavengerPolicy, queryScavengerPolicy);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlSetScrubberStatus, setScrubberStatus);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlQueryScrubberStatus, queryScrubberStatus);

    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlTierPolicy, TierPolicy);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlTierPolicyAudit, TierPolicyAudit);

    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyDLTUpdate, NotifyDLTUpdate);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyDLTClose, NotifyDLTClose);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlStartMigration, StartMigration);

    REGISTER_FDSP_MSG_HANDLER(fpi::AddObjectRefMsg, addObjectRef);

    REGISTER_FDSP_MSG_HANDLER(fpi::ShutdownSMMsg, shutdownSM);

    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifySMStartMigration, migrationInit);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlObjectRebalanceInitialSet, initiateObjectSync);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlObjectRebalanceDeltaSet, syncObjectSet);
}

void
SMSvcHandler::migrationInit(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                boost::shared_ptr<fpi::CtrlNotifySMStartMigration>& migrationMsg)
{
    LOGDEBUG << "Received new migration init message";

    fpi::CtrlNotifyMigrationStatusPtr msg(new fpi::CtrlNotifyMigrationStatus());
    msg->status.DLT_version = migrationMsg->DLT_version;
    msg->status.context = 0;
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyMigrationStatus), *msg);

    // TODO(xxx): We need to be sure to unpack the source -> token list
    // pairs correctly.
    // Source will be an i64, and tokenids within each list will be i32s
    // because Thrift has no notion of unsigned ints. These will need to
    // be cast back to the unsigned equivalents
}

void
SMSvcHandler::initiateObjectSync(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                boost::shared_ptr<fpi::CtrlObjectRebalanceInitialSet>& initialObjSet)
{
    LOGDEBUG << "Initiate Object Sync";
}

void
SMSvcHandler::syncObjectSet(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                boost::shared_ptr<fpi::CtrlObjectRebalanceDeltaSet>& deltaObjSet)
{
    LOGDEBUG << "Sync Object Set";
}

void SMSvcHandler::shutdownSM(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
        boost::shared_ptr<fpi::ShutdownSMMsg>& shutdownMsg) {
    LOGDEBUG << "Received shutdown message... shuttting down...";
    objStorMgr->~ObjectStorMgr();
}

void SMSvcHandler::StartMigration(boost::shared_ptr<fpi::AsyncHdr>& hdr,
        boost::shared_ptr<fpi::CtrlStartMigration>& startMigration) {
    LOGDEBUG << "Start migration handler called";
    objStorMgr->tok_migrated_for_dlt_ = false;
    LOGNORMAL << "Token copy complete";

    // TODO(brian): Do we need a write lock here?
    DLTManagerPtr dlt_mgr = objStorMgr->omClient->getDltManager();
    dlt_mgr->addSerializedDLT(startMigration->dlt_data.dlt_data,
            startMigration->dlt_data.dlt_type);

    LOGDEBUG << "Added serialized dlt to dlt mgr; sending async response";
    fpi::CtrlNotifyMigrationStatusPtr msg(new fpi::CtrlNotifyMigrationStatus());
    msg->status.DLT_version = objStorMgr->omClient->getDltVersion();
    msg->status.context = 0;
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyMigrationStatus), *msg);
    LOGDEBUG << "Async response sent";
}

void SMSvcHandler::queryScrubberStatus(boost::shared_ptr<fpi::AsyncHdr> &hdr,
        fpi::CtrlQueryScrubberStatusPtr &scrub_msg) {
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
        fpi::CtrlSetScrubberStatusPtr &scrub_msg) {
    Error err(ERR_OK);
    fpi::CtrlSetScrubberStatusRespPtr resp(new fpi::CtrlSetScrubberStatusResp());
    LOGNORMAL << " receive scrubber cmd " << scrub_msg->scrubber_status;
    SmScrubberActionCmd scrubCmd(scrub_msg->scrubber_status);
    err = objStorMgr->objectStore->scavengerControlCmd(&scrubCmd);
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlSetScrubberStatusResp), *resp);
}

void SMSvcHandler::queryScavengerProgress(boost::shared_ptr<fpi::AsyncHdr> &hdr,
        fpi::CtrlQueryScavengerProgressPtr &query_msg) {
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
                                      fpi::CtrlSetScavengerPolicyPtr &policy_msg) {
    fpi::CtrlSetScavengerPolicyRespPtr resp(new fpi::CtrlSetScavengerPolicyResp());
    Error err(ERR_OK);
    SmScavengerSetPolicyCmd scavCmd(policy_msg);
    err = objStorMgr->objectStore->scavengerControlCmd(&scavCmd);

    hdr->msg_code = static_cast<int32_t>(err.GetErrno());
    GLOGDEBUG << "Setting scavenger policy " << err;
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlSetScavengerPolicyResp), *resp);
}

void SMSvcHandler::queryScavengerPolicy(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                                        fpi::CtrlQueryScavengerPolicyPtr &query_msg) {
    fpi::CtrlQueryScavengerPolicyRespPtr resp(new fpi::CtrlQueryScavengerPolicyResp());
    SmScavengerGetPolicyCmd scavCmd(resp);
    Error err = objStorMgr->objectStore->scavengerControlCmd(&scavCmd);

    hdr->msg_code = static_cast<int32_t>(err.GetErrno());
    GLOGDEBUG << "Got scavenger policy " << resp->dsk_threshold1;
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlQueryScavengerPolicyResp), *resp);
}

void SMSvcHandler::queryScavengerStatus(boost::shared_ptr<fpi::AsyncHdr> &hdr,
                                        fpi::CtrlQueryScavengerStatusPtr &query_msg) {
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

    DBG(FLAG_CHECK_RETURN_VOID(common_drop_async_resp > 0));
    DBG(FLAG_CHECK_RETURN_VOID(sm_drop_gets > 0));
    fiu_do_on("svc.drop.getobject", return);
#if 0
    fiu_do_on("svc.uturn.getobject",
              auto getReq = new SmIoGetObjectReq(getObjMsg); \
              boost::shared_ptr<GetObjectResp> resp = boost::make_shared<GetObjectResp>(); \
              getReq->getObjectNetResp = resp; \
              getObjectCb(asyncHdr, ERR_OK, getReq); return;);
#endif
    fiu_do_on("svc.uturn.getobject", \
              mockHandler->schedule(mockTimeoutUs, \
                                    std::bind(MockSMCallbacks::mockGetCb, \
                                              asyncHdr)); \
              return;);

    Error err(ERR_OK);
    auto getReq = new SmIoGetObjectReq(getObjMsg);
    getReq->io_type = FDS_SM_GET_OBJECT;
    getReq->setVolId(getObjMsg->volume_id);
    getReq->setObjId(ObjectID(getObjMsg->data_obj_id.digest));
    getReq->obj_data.obj_id = getObjMsg->data_obj_id;
    // perf-trace related data
    getReq->perfNameStr = "volume:" + std::to_string(getObjMsg->volume_id);
    getReq->opReqFailedPerfEventType = SM_GET_OBJ_REQ_ERR;
    getReq->opReqLatencyCtx.type = SM_E2E_GET_OBJ_REQ;
    getReq->opReqLatencyCtx.name = getReq->perfNameStr;
    getReq->opReqLatencyCtx.reset_volid(getObjMsg->volume_id);
    getReq->opLatencyCtx.type = SM_GET_IO;
    getReq->opLatencyCtx.name = getReq->perfNameStr;
    getReq->opLatencyCtx.reset_volid(getObjMsg->volume_id);
    getReq->opQoSWaitCtx.type = SM_GET_QOS_QUEUE_WAIT;
    getReq->opQoSWaitCtx.name = getReq->perfNameStr;
    getReq->opQoSWaitCtx.reset_volid(getObjMsg->volume_id);

    getReq->response_cb = std::bind(
        &SMSvcHandler::getObjectCb, this,
        asyncHdr,
        std::placeholders::_1, std::placeholders::_2);

    // start measuring E2E latency
    PerfTracer::tracePointBegin(getReq->opReqLatencyCtx);

    err = objStorMgr->enqueueMsg(getReq->getVolId(), getReq);
    if (err != fds::ERR_OK) {
        fds_assert(!"Hit an error in enqueing");
        LOGERROR << "Failed to enqueue to SmIoReadObjectMetadata to StorMgr.  Error: "
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
                         getReq->getVolId(), getReq->perfNameStr);
    }

    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::GetObjectResp), *resp);
    delete getReq;
}

void SMSvcHandler::putObject(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                             boost::shared_ptr<fpi::PutObjectMsg>& putObjMsg)  // NOLINT
{
    if ((objStorMgr->testUturnAll == true) ||
        (objStorMgr->testUturnPutObj == true)) {
        LOGDEBUG << "Uturn testing put object "
                 << fds::logString(*asyncHdr) << fds::logString(*putObjMsg);
        putObjectCb(asyncHdr, ERR_OK, NULL);
        return;
    }

    DBG(GLOGDEBUG << fds::logString(*asyncHdr) << fds::logString(*putObjMsg));

    DBG(FLAG_CHECK_RETURN_VOID(common_drop_async_resp > 0));
    DBG(FLAG_CHECK_RETURN_VOID(sm_drop_puts > 0));
    fiu_do_on("svc.drop.putobject", return);
    // fiu_do_on("svc.uturn.putobject", putObjectCb(asyncHdr, ERR_OK, NULL); return;);
    fiu_do_on("svc.uturn.putobject", \
              mockHandler->schedule(mockTimeoutUs, \
                                    std::bind(MockSMCallbacks::mockPutCb, \
                                              asyncHdr)); \
              return;);

    Error err(ERR_OK);
    auto putReq = new SmIoPutObjectReq(putObjMsg);
    putReq->io_type = FDS_SM_PUT_OBJECT;
    putReq->setVolId(putObjMsg->volume_id);
    putReq->origin_timestamp = putObjMsg->origin_timestamp;
    putReq->setObjId(ObjectID(putObjMsg->data_obj_id.digest));
    // perf-trace related data
    putReq->perfNameStr = "volume:" + std::to_string(putObjMsg->volume_id);
    putReq->opReqFailedPerfEventType = SM_PUT_OBJ_REQ_ERR;
    putReq->opReqLatencyCtx.type = SM_E2E_PUT_OBJ_REQ;
    putReq->opReqLatencyCtx.name = putReq->perfNameStr;
    putReq->opReqLatencyCtx.reset_volid(putObjMsg->volume_id);
    putReq->opLatencyCtx.type = SM_PUT_IO;
    putReq->opLatencyCtx.name = putReq->perfNameStr;
    putReq->opLatencyCtx.reset_volid(putObjMsg->volume_id);
    putReq->opQoSWaitCtx.type = SM_PUT_QOS_QUEUE_WAIT;
    putReq->opQoSWaitCtx.name = putReq->perfNameStr;
    putReq->opQoSWaitCtx.reset_volid(putObjMsg->volume_id);

    putReq->response_cb= std::bind(
        &SMSvcHandler::putObjectCb, this,
        asyncHdr,
        std::placeholders::_1, std::placeholders::_2);

    // start measuring E2E latency
    PerfTracer::tracePointBegin(putReq->opReqLatencyCtx);

    err = objStorMgr->enqueueMsg(putReq->getVolId(), putReq);
    if (err != fds::ERR_OK) {
        fds_assert(!"Hit an error in enqueing");
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
                             putReq->getVolId(), putReq->perfNameStr);
        }
    }

    auto resp = boost::make_shared<fpi::PutObjectRspMsg>();
    asyncHdr->msg_code = static_cast<int32_t>(err.GetErrno());
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::PutObjectRspMsg), *resp);

    if ((objStorMgr->testUturnAll == true) ||
        (objStorMgr->testUturnPutObj == true)) {
        fds_verify(putReq == NULL);
        return;
    }

    delete putReq;
}

void SMSvcHandler::deleteObject(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                boost::shared_ptr<fpi::DeleteObjectMsg>& expObjMsg)
{
    DBG(GLOGDEBUG << fds::logString(*asyncHdr) << fds::logString(*expObjMsg));
    Error err(ERR_OK);

    auto delReq = new SmIoDeleteObjectReq();

    // Set delReq stuffs
    delReq->io_type = FDS_SM_DELETE_OBJECT;

    delReq->setVolId(expObjMsg->volId);
    delReq->origin_timestamp = expObjMsg->origin_timestamp;
    delReq->setObjId(ObjectID(expObjMsg->objId.digest));
    // perf-trace related data
    delReq->perfNameStr = "volume:" + std::to_string(expObjMsg->volId);
    delReq->opReqFailedPerfEventType = SM_DELETE_OBJ_REQ_ERR;
    delReq->opReqLatencyCtx.type = SM_E2E_DELETE_OBJ_REQ;
    delReq->opReqLatencyCtx.name = delReq->perfNameStr;
    delReq->opLatencyCtx.type = SM_DELETE_IO;
    delReq->opLatencyCtx.name = delReq->perfNameStr;
    delReq->opQoSWaitCtx.type =SM_DELETE_QOS_QUEUE_WAIT;
    delReq->opQoSWaitCtx.name = delReq->perfNameStr;

    delReq->response_cb = std::bind(
        &SMSvcHandler::deleteObjectCb, this,
        asyncHdr,
        std::placeholders::_1, std::placeholders::_2);

    // start measuring E2E latency
    PerfTracer::tracePointBegin(delReq->opReqLatencyCtx);

    err = objStorMgr->enqueueMsg(delReq->getVolId(), delReq);
    if (err != fds::ERR_OK) {
        fds_assert(!"Hit an error in enqueing");
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
    PerfTracer::tracePointEnd(del_req->opReqLatencyCtx);
    if (!err.ok()) {
        PerfTracer::incr(del_req->opReqFailedPerfEventType,
                         del_req->getVolId(), del_req->perfNameStr);
    }

    auto resp = boost::make_shared<fpi::DeleteObjectRspMsg>();
    asyncHdr->msg_code = static_cast<int32_t>(err.GetErrno());
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::DeleteObjectRspMsg), *resp);

    delete del_req;
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
    fds_volid_t volumeId = vol_msg->vol_desc.volUUID;
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
}

// NotifyRmVol
// -----------
//
void
SMSvcHandler::NotifyRmVol(boost::shared_ptr<fpi::AsyncHdr>            &hdr,
                          boost::shared_ptr<fpi::CtrlNotifyVolRemove> &vol_msg)
{
    fds_volid_t volumeId = vol_msg->vol_desc.volUUID;
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
    fds_volid_t volumeId = vol_msg->vol_desc.volUUID;
    VolumeDesc vdbc(vol_msg->vol_desc), * vdb = &vdbc;
    GLOGNOTIFY << "Received modify for vol "
               << "[" << std::hex << volumeId << std::dec << ", "
               << vdb->getName() << "]";

    StorMgrVolume * vol = objStorMgr->getVol(volumeId);
    fds_assert(vol != NULL);
    if (vol->voldesc->mediaPolicy != vdb->mediaPolicy) {
        GLOGWARN << "Modify volume requested to modify media policy "
                 << "- Not supported yet! Not modifying media policy";
    }

    vol->voldesc->modifyPolicyInfo(vdb->iops_min, vdb->iops_max, vdb->relativePrio);
    err = objStorMgr->modVolQos(vol->getVolId(),
                                vdb->iops_min, vdb->iops_max, vdb->relativePrio);
    if ( !err.ok() )  {
        GLOGERROR << "Modify volume policy failed for vol " << vdb->getName()
                  << std::hex << volumeId << std::dec << " error: "
                  << err.GetErrstr();
    }
    hdr->msg_code = err.GetErrno();
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyVolMod), *vol_msg);
}

// TierPolicy
// ----------
//
void
SMSvcHandler::TierPolicy(boost::shared_ptr<fpi::AsyncHdr>       &hdr,
                         boost::shared_ptr<fpi::CtrlTierPolicy> &msg)
{
    // LOGNOTIFY
    // << "OMClient received tier policy for vol "
    // << tier->tier_vol_uuid;
    fds_verify(objStorMgr->omc_srv_pol != nullptr);
    fdp::FDSP_TierPolicyPtr tp(new FDSP_TierPolicy(msg->tier_policy));
    objStorMgr->omc_srv_pol->serv_recvTierPolicyReq(tp);
    hdr->msg_code = 0;
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlTierPolicy), *msg);
}

// TierPolicyAudit
// ---------------
//
void
SMSvcHandler::TierPolicyAudit(boost::shared_ptr<fpi::AsyncHdr>            &hdr,
                              boost::shared_ptr<fpi::CtrlTierPolicyAudit> &msg)
{
    // LOGNOTIFY
    // << "OMClient received tier audit policy for vol "
    // << audit->tier_vol_uuid;

    fds_verify(objStorMgr->omc_srv_pol != nullptr);
    fdp::FDSP_TierPolicyAuditPtr ta(new FDSP_TierPolicyAudit(msg->tier_audit));
    objStorMgr->omc_srv_pol->serv_recvTierPolicyAuditReq(ta);
    hdr->msg_code = 0;
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlTierPolicyAudit), *msg);
}

void SMSvcHandler::addObjectRef(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                boost::shared_ptr<fpi::AddObjectRefMsg>& addObjRefMsg) {
    DBG(GLOGDEBUG << fds::logString(*asyncHdr) << " " << fds::logString(*addObjRefMsg));
    Error err(ERR_OK);

    auto addObjRefReq = new SmIoAddObjRefReq(addObjRefMsg);

    // perf-trace related data
    addObjRefReq->perfNameStr = "volume:" + std::to_string(addObjRefMsg->srcVolId);
    addObjRefReq->opReqFailedPerfEventType = SM_ADD_OBJ_REF_REQ_ERR;
    addObjRefReq->opReqLatencyCtx.type = SM_E2E_ADD_OBJ_REF_REQ;
    addObjRefReq->opReqLatencyCtx.name = addObjRefReq->perfNameStr;
    addObjRefReq->opLatencyCtx.type = SM_ADD_OBJ_REF_IO;
    addObjRefReq->opLatencyCtx.name = addObjRefReq->perfNameStr;
    addObjRefReq->opQoSWaitCtx.type = SM_ADD_OBJ_REF_QOS_QUEUE_WAIT;
    addObjRefReq->opQoSWaitCtx.name = addObjRefReq->perfNameStr;

    addObjRefReq->response_cb = std::bind(
        &SMSvcHandler::addObjectRefCb, this,
        asyncHdr,
        std::placeholders::_1, std::placeholders::_2);

    // start measuring E2E latency
    PerfTracer::tracePointBegin(addObjRefReq->opReqLatencyCtx);

    err = objStorMgr->enqueueMsg(addObjRefReq->getSrcVolId(), addObjRefReq);
    if (err != fds::ERR_OK) {
        fds_assert(!"Hit an error in enqueing");
        LOGERROR << "Failed to enqueue to SmIoAddObjRefReq to StorMgr.  Error: "
                 << err;
        addObjectRefCb(asyncHdr, err, addObjRefReq);
    }
}

void SMSvcHandler::addObjectRefCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                                  const Error &err, SmIoAddObjRefReq* addObjRefReq) {
    DBG(GLOGDEBUG << fds::logString(*asyncHdr));

    // E2E latency end
    PerfTracer::tracePointEnd(addObjRefReq->opReqLatencyCtx);
    if (!err.ok()) {
        PerfTracer::incr(addObjRefReq->opReqFailedPerfEventType,
                         addObjRefReq->getVolId(), addObjRefReq->perfNameStr);
    }

    auto resp = boost::make_shared<fpi::AddObjectRefRspMsg>();
    asyncHdr->msg_code = static_cast<int32_t>(err.GetErrno());
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::AddObjectRefRspMsg), *resp);

    delete addObjRefReq;
}

void
SMSvcHandler::NotifyScavenger(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                              boost::shared_ptr<fpi::CtrlNotifyScavenger> &msg)
{
    Error err(ERR_OK);
    LOGNORMAL << " receive scavenger cmd " << msg->scavenger.cmd;
    SmScavengerActionCmd scavCmd(msg->scavenger.cmd);
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
    LOGNOTIFY << "Received new DLT commit version  "
              << dlt->dlt_data.dlt_type;
    err = objStorMgr->omClient->updateDlt(dlt->dlt_data.dlt_type, dlt->dlt_data.dlt_data);
    fds_assert(err.ok());
    objStorMgr->handleDltUpdate();

    LOGNOTIFY << "Sending DLT commit response to OM";
    hdr->msg_code = err.GetErrno();
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::EmptyMsg), fpi::EmptyMsg());
}

// NotifyDLTClose
// ---------------
//
void
SMSvcHandler::NotifyDLTClose(boost::shared_ptr<fpi::AsyncHdr> &hdr,
        boost::shared_ptr<fpi::CtrlNotifyDLTClose> &dlt)
{
    MigSvcSyncCloseReqPtr close_req(new MigSvcSyncCloseReq());
    close_req->sync_close_ts = get_fds_timestamp_ms();

    FdsActorRequestPtr close_far(new FdsActorRequest(
            FAR_ID(MigSvcSyncCloseReq), close_req));

    objStorMgr->migrationSvc_->send_actor_request(close_far);

    GLOGNORMAL << "Received ioclose. Time: " << close_req->sync_close_ts;

    /* It's possible no tokens were migrated.  In this we case we simulate
     * MIGRATION_OP_COMPLETE.
     */
    if (objStorMgr->tok_migrated_for_dlt_ == false) {
        LOGNORMAL << "Token migration complete";
        LOGNORMAL << objStorMgr->migrationSvc_->mig_cntrs.toString();
        sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::EmptyMsg), fpi::EmptyMsg());
        objStorMgr->tok_migrated_for_dlt_ = false;
    }
    // sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::EmptyMsg), fpi::EmptyMsg());
}

}  // namespace fds
