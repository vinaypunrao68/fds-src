/*
 * Copyright 2013 Formation Data Systems, Inc.
 *
 * Template to write probe adapter.  Replace Thrpool with your namespace.
 */
#include <dm-probe.h>
#include <string>
#include <list>
#include <utest-types.h>

namespace fds {

probe_mod_param_t Dm_probe_param =
{
    .pr_stat_cnt     = 0,
    .pr_inj_pts_cnt  = 0,
    .pr_inj_act_cnt  = 0,
    .pr_max_sec_tout = 0
};

Dm_ProbeMod gl_Dm_ProbeMod("Data Manager Client Probe Adapter",
                           &Dm_probe_param, nullptr);

// extern DataMgr *dataMgr;

/**
 * Global map of written objects used for verification.
 * This is global so that all probemod objects can access
 * the same map. When a response is received, we don't know
 * which probemod sent it.
 */
std::unordered_map<ObjectID, std::string, ObjectHash> writtenObjs;
fds_mutex objMapLock("Written object map lock");

// pr_new_instance
// ---------------
//
ProbeMod *
Dm_ProbeMod::pr_new_instance()
{
    Dm_ProbeMod *spm = new Dm_ProbeMod("Dm probe Client Inst", &Dm_probe_param, NULL);
    spm->mod_init(mod_params);
    spm->mod_startup();

    return spm;
}

void Dm_ProbeMod::schedule(const OpParams &info)
{
    if (dm_uuid.svc_uuid == 0) {
        std::cout << "Dm uuid = " << dm_uuid.svc_uuid << std::endl;
        Platform::plf_get_my_dm_svc_uuid(&dm_uuid);
    }

    if (info.op == "update") {
        sendUpdate(info);
    } else if (info.op == "delete") {
        sendDelete(info);
    } else if (info.op == "query") {
        sendQuery(info);
    } else if (info.op == "start") {
        sendStartTx(info);
    } else if (info.op == "commit") {
        sendCommitTx(info);
    } else if (info.op == "abort") {
        sendAbortTx(info);
    }
}

void
Dm_ProbeMod::sendStartTx(const OpParams &updateParams)
{
    std::cout << "Doing Start Tx" << std::endl;
    StartBlobTxMsgPtr stTxMsg(new StartBlobTxMsg());
    stTxMsg->volume_id = updateParams.volId;
    stTxMsg->blob_name = updateParams.blobName;
    stTxMsg->blob_version = updateParams.blobVersion;
    stTxMsg->blob_mode = updateParams.blobMode;
    stTxMsg->txId = updateParams.txId;
    stTxMsg->dmt_version = 1;

    auto asyncStartTxReq = gSvcRequestPool->newEPSvcRequest(dm_uuid);
    asyncStartTxReq->setPayload(FDSP_MSG_TYPEID(fpi::StartBlobTxMsg), stTxMsg);
    asyncStartTxReq->onResponseCb(RESPONSE_MSG_HANDLER(Dm_ProbeMod::genericResp));
    asyncStartTxReq->invoke();
}

void
Dm_ProbeMod::genericResp(EPSvcRequest* svcReq,
                         const Error& error,
                         boost::shared_ptr<std::string> payload)
{
    LOGDEBUG << "genericTxResp - RECEIVED RESPONSE: " << svcReq->logString()
             << " Error code: " << error.GetErrno() << std::endl;
}

void
Dm_ProbeMod::sendCommitTx(const OpParams &updateParams)
{
    std::cout << "Doing Commit Tx" << std::endl;

    CommitBlobTxMsgPtr stCommitMsg(new CommitBlobTxMsg());
    stCommitMsg->volume_id = updateParams.volId;
    stCommitMsg->blob_name = updateParams.blobName;
    stCommitMsg->blob_version = updateParams.blobVersion;
    stCommitMsg->txId = updateParams.txId;
    stCommitMsg->dmt_version = 1;
}

void
Dm_ProbeMod::sendAbortTx(const OpParams &updateParams)
{
    std::cout << "Doing Abort Tx" << std::endl;

    AbortBlobTxMsgPtr stBlobTxMsg(new AbortBlobTxMsg());
    stBlobTxMsg->blob_name = updateParams.blobName;
    stBlobTxMsg->blob_version = updateParams.blobVersion;
    stBlobTxMsg->volume_id = updateParams.volId;
    stBlobTxMsg->txId = updateParams.txId;

    auto asyncAbortBlobTxReq = gSvcRequestPool->newEPSvcRequest(dm_uuid);

    asyncAbortBlobTxReq->setPayload(FDSP_MSG_TYPEID(fpi::AbortBlobTxMsg), stBlobTxMsg);
    asyncAbortBlobTxReq->onResponseCb(RESPONSE_MSG_HANDLER(Dm_ProbeMod::genericResp));
    asyncAbortBlobTxReq->invoke();
}

void
Dm_ProbeMod::sendUpdate(const OpParams &updateParams)
{
    std::cout << "Doing an update" << std::endl;

    UpdateCatalogMsgPtr stUpdateMsg(new UpdateCatalogMsg());
    stUpdateMsg->volume_id = updateParams.volId;
    stUpdateMsg->blob_name = updateParams.blobName;
    stUpdateMsg->blob_version = updateParams.blobVersion;
    stUpdateMsg->txId = updateParams.txId;
    updateParams.obj_list->toFdspPayload(stUpdateMsg->obj_list);

    auto asyncUpdateCatReq = gSvcRequestPool->newEPSvcRequest(dm_uuid);
    asyncUpdateCatReq->setPayload(FDSP_MSG_TYPEID(fpi::UpdateCatalogMsg), stUpdateMsg);
    asyncUpdateCatReq->onResponseCb(RESPONSE_MSG_HANDLER(Dm_ProbeMod::genericResp));
    asyncUpdateCatReq->invoke();
}

void
Dm_ProbeMod::sendQuery(const OpParams &queryParams)
{
    std::cout << "Doing a query" << std::endl;

    QueryCatalogMsgPtr stQueryMsg(new QueryCatalogMsg());
    stQueryMsg->volume_id = queryParams.volId;
    stQueryMsg->blob_name = queryParams.blobName;
    stQueryMsg->blob_version = queryParams.blobVersion;
    queryParams.obj_list->toFdspPayload(stQueryMsg->obj_list);
    queryParams.meta_list.toFdspPayload(stQueryMsg->meta_list);

    auto asyncQueryCatReq = gSvcRequestPool->newEPSvcRequest(dm_uuid);
    asyncQueryCatReq->setPayload(FDSP_MSG_TYPEID(fpi::QueryCatalogMsg), stQueryMsg);
    asyncQueryCatReq->invoke();
}

void
Dm_ProbeMod::sendDelete(const OpParams &queryParams)
{
    std::cout << "Doing a delete" << std::endl;

    DeleteBlobMsgPtr stDelMsg(new DeleteBlobMsg());
    stDelMsg->txId = queryParams.txId;
    stDelMsg->volume_id = queryParams.volId;
    stDelMsg->blob_name = queryParams.blobName;
    stDelMsg->blob_version = queryParams.blobVersion;

    auto asyncDelReq = gSvcRequestPool->newEPSvcRequest(dm_uuid);
    asyncDelReq->setPayload(FDSP_MSG_TYPEID(fpi::DeleteBlobMsg), stDelMsg);
    asyncDelReq->invoke();
}

// pr_intercept_request
// --------------------
//
void
Dm_ProbeMod::pr_intercept_request(ProbeRequest *req)
{
}

// pr_put
// ------
//
void
Dm_ProbeMod::pr_put(ProbeRequest *probe)
{
}

// pr_get
// ------
//
void
Dm_ProbeMod::pr_get(ProbeRequest *req)
{
}

// pr_delete
// ---------
//
void
Dm_ProbeMod::pr_delete(ProbeRequest *req)
{
}

// pr_verify_request
// -----------------
//
void
Dm_ProbeMod::pr_verify_request(ProbeRequest *req)
{
}

// pr_gen_report
// -------------
//
void
Dm_ProbeMod::pr_gen_report(std::string *out)
{
}

// mod_init
// --------
//
int
Dm_ProbeMod::mod_init(SysParams const *const param)
{
    Module::mod_init(param);
    return 0;
}

// mod_startup
// -----------
//
void
Dm_ProbeMod::mod_startup()
{
    JsObjManager  *mgr;
    JsObjTemplate *svc;

    std::cout << "mod_startup called" << std::endl;

    if (pr_parent != NULL) {
        mgr = pr_parent->pr_get_obj_mgr();
        svc = mgr->js_get_template("Run-Input");
        svc->js_register_template(new UT_DmWorkloadTemplate(mgr));
    }
}

// mod_shutdown
// ------------
//
void
Dm_ProbeMod::mod_shutdown()
{
}

// ----------------------------------------------------------------------------
// Dm client data path
// ----------------------------------------------------------------------------
JsObject *
UT_ObjectOp::js_exec_obj(JsObject *parent,
                         JsObjTemplate *templ,
                         JsObjOutput *out)
{
    fds_uint32_t numOps = parent->js_array_size();
    UT_ObjectOp *node;
    Dm_ProbeMod::OpParams *info;
    for (fds_uint32_t i = 0; i < numOps; i++) {
        node = static_cast<UT_ObjectOp *>((*parent)[i]);
        info = node->dm_ops();
        std::cout << "Doing a " << info->op << " for blob "
                  << info->blobName << std::endl;

        fds::gl_Dm_ProbeMod.schedule(*info);
    }
    return this;
}

}  // namespace fds
