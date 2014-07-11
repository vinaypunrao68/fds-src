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

extern DataMgr *dataMgr;

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
    Dm_ProbeMod *spm = new Dm_ProbeMod("Sm Client Inst", &Dm_probe_param, NULL);
    spm->mod_init(mod_params);
    spm->mod_startup();

    return spm;
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


void
Dm_ProbeMod::sendStartTx(const OpParams &updateParams)
{
    std::cout << "Doing Start Tx" << std::endl;

    auto dmBlobTxReq = new DmIoStartBlobTx(updateParams.volId,
                                           updateParams.blobName,
                                           updateParams.blobVersion);
    dmBlobTxReq->ioBlobTxDesc =
         BlobTxId::ptr(new BlobTxId(updateParams.txId));
    dataMgr->scheduleStartBlobTxSvc(dmBlobTxReq);
}

void
Dm_ProbeMod::sendCommitTx(const OpParams &updateParams)
{
    std::cout << "Doing Commit Tx" << std::endl;

    auto dmBlobTxReq = new DmIoCommitBlobTx(updateParams.volId,
                                            updateParams.blobName,
                                            updateParams.blobVersion);
    dmBlobTxReq->ioBlobTxDesc =
         BlobTxId::ptr(new BlobTxId(updateParams.txId));
    dataMgr->scheduleCommitBlobTxSvc(dmBlobTxReq);
}

void
Dm_ProbeMod::sendAbortTx(const OpParams &updateParams)
{
    std::cout << "Doing Abort Tx" << std::endl;


    auto dmBlobTxReq = new DmIoAbortBlobTx(updateParams.volId,
                                          updateParams.blobName,
                                          updateParams.blobVersion);
    dmBlobTxReq->ioBlobTxDesc =
         BlobTxId::ptr(new BlobTxId(updateParams.txId));
    dataMgr->scheduleAbortBlobTxSvc(dmBlobTxReq);
}

void
Dm_ProbeMod::sendUpdate(const OpParams &updateParams)
{
    std::cout << "Doing an update" << std::endl;

    // blobObjInfo.offset = updateParams.blobOffset;
    // blobObjInfo.blob_end = updateParams.endBuf;
    // blobObjInfo.data_obj_id.digest = std::string(
    //     (const char *)updateParams.objectId.GetId(),
    //     (size_t)updateParams.objectId.GetLen());

    // create blob object list
    BlobObjList::ptr obj_list(new BlobObjList());
    for (BlobObjList::const_iter cit = updateParams.obj_list.cbegin();
         cit != updateParams.obj_list.cend();
         ++cit) {
        obj_list->updateObject(cit->first, cit->second);
    }
    std::cout << "Will call putBlob for "  << *obj_list;

    auto dmUpdCatReq = new DmIoUpdateCat(updateParams.volId,
                                         updateParams.blobName,
                                         updateParams.blobVersion);
    // dmUpdCatReq->obj_list.push_back(blobObjInfo);

    BlobNode *bnode = NULL;
    Error err = dataMgr->updateCatalogProcessSvc(dmUpdCatReq, &bnode);
    fds_verify(err == ERR_OK);
    fds_verify(bnode != NULL);
}

// pr_get
// ------
//
void
Dm_ProbeMod::pr_get(ProbeRequest *req)
{
}

void
Dm_ProbeMod::sendQuery(const OpParams &queryParams)
{
    std::cout << "Doing a query" << std::endl;

    auto dmQryReq = new DmIoQueryCat(queryParams.volId,
                                     queryParams.blobName,
                                     queryParams.blobVersion);

    BlobNode *bnode = NULL;
    Error err =dataMgr-> queryCatalogProcess(dmQryReq, &bnode);
    if (err == ERR_BLOB_NOT_FOUND) {
        std::cout << "Blob " << dmQryReq->blob_name
                  << " was NOT found" << std::endl;
    } else {
        fds_verify(err == ERR_OK);
        fds_verify(bnode != NULL);
        std::cout << "Queried blob " << dmQryReq->blob_name
                  << " OK with size " << bnode->blob_size
                  << " and " << bnode->obj_list.size()
                  << " entries" << std::endl;
    }

    if (bnode != NULL) {
        delete bnode;
    }
    delete dmQryReq;
}

// pr_delete
// ---------
//
void
Dm_ProbeMod::pr_delete(ProbeRequest *req)
{
}

void
Dm_ProbeMod::sendDelete(const OpParams &deleteParams)
{
    std::cout << "Doing a delete" << std::endl;

    auto dmDelCatReq = new DmIoDeleteCat(deleteParams.volId,
                                         deleteParams.blobName,
                                         deleteParams.blobVersion);

    BlobNode *bnode = NULL;
    // Process the delete blob. The deleted or modified
    // bnode will be allocated and returned on success
    Error err = dataMgr->deleteBlobProcessSvc(dmDelCatReq, &bnode);

    fds_verify(err == ERR_OK);
    fds_verify(bnode != NULL);

    delete bnode;
    delete dmDelCatReq;
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

        if (info->op == "update") {
            gl_Dm_ProbeMod.sendUpdate(*info);
        } else if (info->op == "delete") {
            gl_Dm_ProbeMod.sendDelete(*info);
        } else if (info->op == "query") {
            gl_Dm_ProbeMod.sendQuery(*info);
        } else if (info->op == "start") {
            gl_Dm_ProbeMod.sendStartTx(*info);
        } else if (info->op == "commit") {
            gl_Dm_ProbeMod.sendCommitTx(*info);
        } else if (info->op == "abort") {
            gl_Dm_ProbeMod.sendAbortTx(*info);
        }
    }

    return this;
}

}  // namespace fds
