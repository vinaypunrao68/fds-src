/*
 * Copyright 2014 Formation Data Systems, Inc.
 *
 * CommitLog workload probe adapter.
 */
#include <commit-log-probe.h>
#include <string>
#include <list>
#include <utest-types.h>
#include <fds_types.h>

#include <dm-tvc/OperationJournal.h>

namespace fds {

probe_mod_param_t commit_log_probe_param =
{
    .pr_stat_cnt     = 0,
    .pr_inj_pts_cnt  = 0,
    .pr_inj_act_cnt  = 0,
    .pr_max_sec_tout = 0
};

// Operation log
DmTvcOperationJournal journal(0);

/// Global singleton probe module
CommitLogProbe gl_CommitLogProbe("CommitLog Probe Adapter", &commit_log_probe_param, nullptr);

DmCommitLog gl_DmCommitLogMod("Probe CommitLog", 1, journal);

CommitLogProbe::CommitLogProbe(const std::string &name, probe_mod_param_t *param, Module *owner)
        : ProbeMod(name.c_str(), param, owner) {}

CommitLogProbe::~CommitLogProbe() {}

// pr_new_instance
// ---------------
//
ProbeMod * CommitLogProbe::pr_new_instance() {
    CommitLogProbe *pm = new CommitLogProbe("CommitLog test Inst", &commit_log_probe_param, NULL);
    pm->mod_init(mod_params);
    pm->mod_startup();

    return pm;
}

// pr_intercept_request
// --------------------
//
void CommitLogProbe::pr_intercept_request(ProbeRequest *req) {}

// pr_put
// ------
//
void CommitLogProbe::pr_put(ProbeRequest *probe) {}

// pr_get
// ------
//
void CommitLogProbe::pr_get(ProbeRequest *req) {}

void CommitLogProbe::startTx(const OpParams &startParams) {
    BlobTxId::const_ptr id = startParams.txId;
    Error err = gl_DmCommitLogMod.startTx(id, startParams.blobName, blob::TRUNCATE);
    fds_verify(err == ERR_OK);
}

void CommitLogProbe::updateTx(const OpParams &updateParams) {
    BlobTxId::const_ptr id = updateParams.txId;
    BlobObjList::const_ptr objList(new BlobObjList(updateParams.objList));
    Error err = gl_DmCommitLogMod.updateTx(id, objList);
    fds_verify(err == ERR_OK);
}

void CommitLogProbe::updateMetaTx(const OpParams &updateParams) {
    BlobTxId::const_ptr id = updateParams.txId;
    MetaDataList::const_ptr metaList(new MetaDataList(updateParams.metaList));
    Error err = gl_DmCommitLogMod.updateTx(id, metaList);
    fds_verify(err == ERR_OK);
}

void CommitLogProbe::commitTx(const OpParams &commitParams) {
    Error err;
    BlobTxId::const_ptr id = commitParams.txId;
    CommitLogTx::const_ptr clTx = gl_DmCommitLogMod.commitTx(id, err);

    fds_verify(err == ERR_OK);
    fds_verify(*clTx->txDesc == *id);

    // TODO(umesh): additional verifications on updates
}

void CommitLogProbe::abortTx(const OpParams &abortParams) {
    BlobTxId::const_ptr id = abortParams.txId;
    Error err = gl_DmCommitLogMod.rollbackTx(id);
    fds_verify(err == ERR_OK);
}

void CommitLogProbe::purgeTx(const OpParams &purgeParams) {
    // BlobTxId::const_ptr id = purgeParams.txId;
    // Error err = gl_DmCommitLogMod.purgeTx(id);
    // fds_verify(err == ERR_OK);
}

// pr_delete
// ---------
//
void CommitLogProbe::pr_delete(ProbeRequest *req) {}

// pr_verify_request
// -----------------
//
void CommitLogProbe::pr_verify_request(ProbeRequest *req) {}

// pr_gen_report
// -------------
//
void CommitLogProbe::pr_gen_report(std::string *out) {}

// mod_init
// --------
//
int CommitLogProbe::mod_init(SysParams const *const param) {
    Module::mod_init(param);
    gl_DmCommitLogMod.mod_init(param);
    return 0;
}

// mod_startup
// -----------
//
void CommitLogProbe::mod_startup() {
    JsObjManager  *mgr;
    JsObjTemplate *svc;

    if (pr_parent != NULL) {
        mgr = pr_parent->pr_get_obj_mgr();
        svc = mgr->js_get_template("Run-Input");
        svc->js_register_template(new CommitLogWorkloadTemplate(mgr));
    }
    gl_DmCommitLogMod.mod_startup();
}

// mod_shutdown
// ------------
//
void
CommitLogProbe::mod_shutdown() {
    gl_DmCommitLogMod.mod_shutdown();
}

/**
 * Workload dispatcher
 */
JsObject *
CommitLogObjectOp::js_exec_obj(JsObject *parent,
                         JsObjTemplate *templ,
                         JsObjOutput *out) {
    fds_uint32_t numOps = parent->js_array_size();
    CommitLogObjectOp *node;
    CommitLogProbe::OpParams *info;

    for (fds_uint32_t i = 0; i < numOps; i++) {
        node = static_cast<CommitLogObjectOp *>((*parent)[i]);
        info = node->commit_log_ops();
        std::cout << "Doing a " << info->op << " for blob "
                  << info->blobName << std::endl;

        if (info->op == "startTx") {
            gl_CommitLogProbe.startTx(*info);
        } else if (info->op == "updateTx") {
            gl_CommitLogProbe.updateTx(*info);
        } else if (info->op == "updateMetaTx") {
            gl_CommitLogProbe.updateMetaTx(*info);
        } else if (info->op == "commitTx") {
            gl_CommitLogProbe.commitTx(*info);
        } else if (info->op == "abortTx") {
            gl_CommitLogProbe.abortTx(*info);
        } else if (info->op == "purgeTx") {
            gl_CommitLogProbe.purgeTx(*info);
        } else {
            fds_panic("Unknown operation %s!", info->op.c_str());
        }
    }

    return this;
}

}  // namespace fds
