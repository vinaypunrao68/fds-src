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

void
Dm_ProbeMod::InitDmMsgHdr(const fdspi::FDSP_MsgHdrTypePtr& msg_hdr)
{
    msg_hdr->msg_code = FDSP_MSG_UPDATE_CAT_OBJ_REQ;
    msg_hdr->msg_id =  1;

    msg_hdr->major_ver = 0xa5;
    msg_hdr->minor_ver = 0x5a;

    msg_hdr->num_objects = 1;
    msg_hdr->frag_len = 0;
    msg_hdr->frag_num = 0;

    msg_hdr->tennant_id = 0;
    msg_hdr->local_domain_id = 0;

    msg_hdr->src_id = FDSP_STOR_HVISOR;
    msg_hdr->dst_id = FDSP_DATA_MGR;

    msg_hdr->src_node_name = myName;

    msg_hdr->err_code = ERR_OK;
    msg_hdr->result = FDSP_ERR_OK;

    msg_hdr->req_cookie = transId;
    transId++;
}

// pr_put
// ------
//
void
Dm_ProbeMod::pr_put(ProbeRequest *probe)
{
}

void
Dm_ProbeMod::sendUpdate(const OpParams &updateParams)
{
    std::cout << "Doing an update" << std::endl;
    fdspi::FDSP_MsgHdrTypePtr msgHdr(new fdspi::FDSP_MsgHdrType);
    InitDmMsgHdr(msgHdr);
    msgHdr->glob_volume_id = updateParams.volId;
    msgHdr->msg_code       = fdspi::FDSP_MSG_UPDATE_CAT_OBJ_REQ;

    fdspi::FDSP_UpdateCatalogTypePtr upCatReq(new FDSP_UpdateCatalogType);
    upCatReq->blob_name = updateParams.blobName;
    upCatReq->obj_list.clear();
    upCatReq->dm_transaction_id = 1;
    upCatReq->dm_operation      = FDS_DMGR_TXN_STATUS_OPEN;
    // TODO(Andrew): Actually set this...
    upCatReq->txDesc.txId       = 0;

    fdspi::FDSP_BlobObjectInfo blobObjInfo;
    blobObjInfo.offset = updateParams.blobOffset;
    blobObjInfo.size = updateParams.objectLen;
    blobObjInfo.data_obj_id.digest = std::string(
        (const char *)updateParams.objectId.GetId(),
        (size_t)updateParams.objectId.GetLen());

    upCatReq->obj_list.push_back(blobObjInfo);
    upCatReq->meta_list.clear();

    // Allocate a dm request using the fdsp message
    DataMgr::dmCatReq *dmUpdReq = new DataMgr::dmCatReq(
        updateParams.volId,
        upCatReq->blob_name,
        upCatReq->dm_transaction_id,
        upCatReq->dm_operation,
        0,  // Source IP is 0
        0,  // Dst IP is 0
        0,  // Source port is 0
        0,  // Dst port is 0
        "0",  // Session UUID is 0
        msgHdr->req_cookie,
        FDS_CAT_UPD,
        upCatReq);

    BlobNode *bnode = NULL;
    Error err = dataMgr->updateCatalogProcess(dmUpdReq, &bnode);
    fds_verify(err == ERR_OK);
    fds_verify(bnode != NULL);

    delete bnode;
    delete dmUpdReq;
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

    // Allocate a dm request using the fdsp message
    DataMgr::dmCatReq *dmQueryReq = new DataMgr::dmCatReq(
        queryParams.volId,
        queryParams.blobName,
        0,  // Dm trans id is 0
        0,  // Dm op id is 0
        0,  // Source IP is 0
        0,  // Dst IP is 0
        0,  // Source port is 0
        0,  // Dst port is 0
        "0",  // Session UUID is 0
        0,  // Request cookie is 0
        FDS_CAT_QRY,
        NULL);
    dmQueryReq->setBlobVersion(queryParams.blobVersion);

    BlobNode *bnode = NULL;
    Error err = dataMgr->queryCatalogProcess(dmQueryReq, &bnode);
    if (err == ERR_BLOB_NOT_FOUND) {
        std::cout << "Blob " << dmQueryReq->blob_name
                  << " was NOT found" << std::endl;
    } else {
        fds_verify(err == ERR_OK);
        fds_verify(bnode != NULL);
        std::cout << "Queried blob " << dmQueryReq->blob_name
                  << " OK with size " << bnode->blob_size
                  << " and " << bnode->obj_list.size()
                  << " entries" << std::endl;
    }

    if (bnode != NULL) {
        delete bnode;
    }
    delete dmQueryReq;
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

    // Allocate a dm request using the fdsp message
    DataMgr::dmCatReq *dmDelReq = new DataMgr::dmCatReq(
        deleteParams.volId,
        deleteParams.blobName,
        0,  // Dm trans id is 0
        0,  // Dm op id is 0
        0,  // Source IP is 0
        0,  // Dst IP is 0
        0,  // Source port is 0
        0,  // Dst port is 0
        "0",  // Session UUID is 0
        0,  // Request cookie is 0
        FDS_DELETE_BLOB,
        NULL);
    dmDelReq->setBlobVersion(deleteParams.blobVersion);

    BlobNode *bnode = NULL;
    Error err = dataMgr->deleteBlobProcess(dmDelReq, &bnode);
    fds_verify(err == ERR_OK);
    fds_verify(bnode != NULL);

    delete bnode;
    delete dmDelReq;
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
        }
    }

    return this;
}

}  // namespace fds
