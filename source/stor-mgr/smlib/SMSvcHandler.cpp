/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <StorMgr.h>
#include <net/net-service-tmpl.hpp>
#include <fdsp_utils.h>
#include <fds_assert.h>
#include <SMSvcHandler.h>
#include <platform/fds_flags.h>
#include <sm-platform.h>
#include <string>

namespace fds {

extern ObjectStorMgr *objStorMgr;

SMSvcHandler::SMSvcHandler()
{
    REGISTER_FDSP_MSG_HANDLER(fpi::GetObjectMsg, getObject);
    REGISTER_FDSP_MSG_HANDLER(fpi::PutObjectMsg, putObject);
    REGISTER_FDSP_MSG_HANDLER(fpi::DeleteObjectMsg, deleteObject);

    REGISTER_FDSP_MSG_HANDLER(fpi::NodeSvcInfo, notifySvcChange);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyVolAdd, NotifyAddVol);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyVolRemove, NotifyRmVol);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyVolMod, NotifyModVol);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlTierPolicy, TierPolicy);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlTierPolicyAudit, TierPolicyAudit);
}

void SMSvcHandler::getObject(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                             boost::shared_ptr<fpi::GetObjectMsg>& getObjMsg)  // NOLINT
{
    DBG(GLOGDEBUG << logString(*asyncHdr) << fds::logString(*getObjMsg));

    DBG(FLAG_CHECK_RETURN_VOID(common_drop_async_resp > 0));
    DBG(FLAG_CHECK_RETURN_VOID(sm_drop_gets > 0));

    Error err(ERR_OK);
    auto read_req = new SmIoReadObjectdata();
    read_req->io_type = FDS_SM_GET_OBJECT;
    read_req->setVolId(getObjMsg->volume_id);
    read_req->setObjId(ObjectID(getObjMsg->data_obj_id.digest));
    read_req->obj_data.obj_id = getObjMsg->data_obj_id;
    // perf-trace related data
    read_req->perfNameStr = "volume:" + std::to_string(getObjMsg->volume_id);
    read_req->opReqFailedPerfEventType = GET_OBJ_REQ_ERR;
    read_req->opReqLatencyCtx.type = GET_OBJ_REQ;
    read_req->opReqLatencyCtx.name = read_req->perfNameStr;
    read_req->opReqLatencyCtx.reset_volid(getObjMsg->volume_id);
    read_req->opLatencyCtx.type = GET_IO;
    read_req->opLatencyCtx.name = read_req->perfNameStr;
    read_req->opLatencyCtx.reset_volid(getObjMsg->volume_id);
    read_req->opTransactionWaitCtx.type = GET_TRANS_QUEUE_WAIT;
    read_req->opTransactionWaitCtx.name = read_req->perfNameStr;
    read_req->opTransactionWaitCtx.reset_volid(getObjMsg->volume_id);
    read_req->opQoSWaitCtx.type = GET_QOS_QUEUE_WAIT;
    read_req->opQoSWaitCtx.name = read_req->perfNameStr;
    read_req->opQoSWaitCtx.reset_volid(getObjMsg->volume_id);


    read_req->smio_readdata_resp_cb = std::bind(
            &SMSvcHandler::getObjectCb, this,
            asyncHdr,
            std::placeholders::_1, std::placeholders::_2);

    err = objStorMgr->enqueueMsg(read_req->getVolId(), read_req);
    if (err != fds::ERR_OK) {
        fds_assert(!"Hit an error in enqueing");
        LOGERROR << "Failed to enqueue to SmIoReadObjectMetadata to StorMgr.  Error: "
                << err;
        getObjectCb(asyncHdr, err, read_req);
    }
}

void SMSvcHandler::getObjectCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                               const Error &err,
                               SmIoReadObjectdata *read_req)
{
    DBG(GLOGDEBUG << fds::logString(*asyncHdr));

    auto resp = boost::make_shared<GetObjectResp>();
    asyncHdr->msg_code = static_cast<int32_t>(err.GetErrno());
    resp->data_obj_len = read_req->obj_data.data.size();
    resp->data_obj = read_req->obj_data.data;
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::GetObjectResp), *resp);

    delete read_req;
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

    Error err(ERR_OK);
    auto put_req = new SmIoPutObjectReq();
    put_req->io_type = FDS_SM_PUT_OBJECT;
    put_req->setVolId(putObjMsg->volume_id);
    put_req->origin_timestamp = putObjMsg->origin_timestamp;
    put_req->setObjId(ObjectID(putObjMsg->data_obj_id.digest));
    put_req->data_obj = std::move(putObjMsg->data_obj);
    // perf-trace related data
    put_req->perfNameStr = "volume:" + std::to_string(putObjMsg->volume_id);
    put_req->opReqFailedPerfEventType = PUT_OBJ_REQ_ERR;
    put_req->opReqLatencyCtx.type = PUT_OBJ_REQ;
    put_req->opReqLatencyCtx.name = put_req->perfNameStr;
    put_req->opReqLatencyCtx.reset_volid(putObjMsg->volume_id);
    put_req->opLatencyCtx.type = PUT_IO;
    put_req->opLatencyCtx.name = put_req->perfNameStr;
    put_req->opLatencyCtx.reset_volid(putObjMsg->volume_id);
    put_req->opTransactionWaitCtx.type = PUT_TRANS_QUEUE_WAIT;
    put_req->opTransactionWaitCtx.name = put_req->perfNameStr;
    put_req->opTransactionWaitCtx.reset_volid(putObjMsg->volume_id);
    put_req->opQoSWaitCtx.type = PUT_QOS_QUEUE_WAIT;
    put_req->opQoSWaitCtx.name = put_req->perfNameStr;
    put_req->opQoSWaitCtx.reset_volid(putObjMsg->volume_id);

    putObjMsg->data_obj.clear();
    put_req->response_cb= std::bind(
            &SMSvcHandler::putObjectCb, this,
            asyncHdr,
            std::placeholders::_1, std::placeholders::_2);

    err = objStorMgr->enqueueMsg(put_req->getVolId(), put_req);
    if (err != fds::ERR_OK) {
        fds_assert(!"Hit an error in enqueing");
        LOGERROR << "Failed to enqueue to SmIoPutObjectReq to StorMgr.  Error: "
                << err;
        putObjectCb(asyncHdr, err, put_req);
    }
}

void SMSvcHandler::putObjectCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                               const Error &err,
                               SmIoPutObjectReq* put_req)
{
    DBG(GLOGDEBUG << fds::logString(*asyncHdr));

    auto resp = boost::make_shared<fpi::PutObjectRspMsg>();
    asyncHdr->msg_code = static_cast<int32_t>(err.GetErrno());
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::PutObjectRspMsg), *resp);

    if ((objStorMgr->testUturnAll == true) ||
        (objStorMgr->testUturnPutObj == true)) {
        fds_verify(put_req == NULL);
        return;
    }

    delete put_req;
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
    delReq->opReqFailedPerfEventType = DELETE_OBJ_REQ_ERR;
    delReq->opReqLatencyCtx.type = DELETE_OBJ_REQ;
    delReq->opReqLatencyCtx.name = delReq->perfNameStr;
    delReq->opLatencyCtx.type = DELETE_IO;
    delReq->opLatencyCtx.name = delReq->perfNameStr;
    delReq->opTransactionWaitCtx.type = DELETE_TRANS_QUEUE_WAIT;
    delReq->opTransactionWaitCtx.name = delReq->perfNameStr;
    delReq->opQoSWaitCtx.type = DELETE_QOS_QUEUE_WAIT;
    delReq->opQoSWaitCtx.name = delReq->perfNameStr;

    delReq->response_cb = std::bind(
        &SMSvcHandler::deleteObjectCb, this,
        asyncHdr,
        std::placeholders::_1, std::placeholders::_2);

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
        err = objStorMgr->regVolQos(vol->getVolId(),
                      static_cast<FDS_VolumeQueue*>(vol->getQueue()));

        if (err.ok()) {
            objStorMgr->createCache(volumeId, 1024 * 1024 * 8, 1024 * 1024 * 256);
        } else {
            // most likely axceeded min iops
            objStorMgr->deregVol(volumeId);
        }
    }
    if (!err.ok()) {
        GLOGERROR << "Registration failed for vol id " << std::hex << volumeId
                  << std::dec << " error: " << err.GetErrstr();
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
    GLOGNOTIFY << "Received delete for vol "
               << "[" << std::hex << volumeId << std::dec << ", "
               << volName << "]";
    objStorMgr->quieseceIOsQos(volumeId);
    objStorMgr->deregVolQos(volumeId);
    objStorMgr->deregVol(volumeId);
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
    addObjRefReq->opReqFailedPerfEventType = ADD_OBJECT_REF_REQ_ERR;
    addObjRefReq->opReqLatencyCtx.type = ADD_OBJECT_REF_REQ;
    addObjRefReq->opReqLatencyCtx.name = addObjRefReq->perfNameStr;
    addObjRefReq->opLatencyCtx.type = ADD_OBJECT_REF_IO;
    addObjRefReq->opLatencyCtx.name = addObjRefReq->perfNameStr;
    addObjRefReq->opTransactionWaitCtx.type = ADD_OBJECT_REF_TRANS_QUEUE_WAIT;
    addObjRefReq->opTransactionWaitCtx.name = addObjRefReq->perfNameStr;
    addObjRefReq->opQoSWaitCtx.type = ADD_OBJECT_REF_QOS_QUEUE_WAIT;
    addObjRefReq->opQoSWaitCtx.name = addObjRefReq->perfNameStr;

    addObjRefReq->response_cb = std::bind(
        &SMSvcHandler::addObjectRefCb, this,
        asyncHdr,
        std::placeholders::_1, std::placeholders::_2);

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

    auto resp = boost::make_shared<fpi::AddObjectRefRspMsg>();
    asyncHdr->msg_code = static_cast<int32_t>(err.GetErrno());
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::AddObjectRefRspMsg), *resp);

    delete addObjRefReq;
}

}  // namespace fds
