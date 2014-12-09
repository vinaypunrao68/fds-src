/*
 * Copyright 2014 Formation Data Systems, Inc.
 *
 * TVC workload probe adapter.
 */

#include <vector>
#include <tvc-probe.h>
#include <string>
#include <list>
#include <utest-types.h>
#include <fds_types.h>
#include <concurrency/ThreadPool.h>

namespace {
fds::fds_threadpool s_threadpool;
fds::DmTimeVolCatalog gl_DmTvcMod("Global DM TVC", s_threadpool);
}

namespace fds {

void commitCallback(const fds::Error & err,
                    blob_version_t,
                    const BlobObjList::const_ptr&,
                    const MetaDataList::const_ptr&) {}

Error expungeObjects(fds_volid_t volId, const std::vector<ObjectID>& objs) {
    return ERR_OK;
}

probe_mod_param_t tvc_probe_param =
{
    .pr_stat_cnt     = 0,
    .pr_inj_pts_cnt  = 0,
    .pr_inj_act_cnt  = 0,
    .pr_max_sec_tout = 0
};

/// Global singleton probe module
TvcProbe gl_TvcProbe("TVC Probe Adapter",
                     &tvc_probe_param,
                     nullptr);

TvcProbe::TvcProbe(const std::string &name,
                   probe_mod_param_t *param,
                   Module *owner)
        : ProbeMod(name.c_str(),
                   param,
                   owner) {
}

TvcProbe::~TvcProbe() {
}

// pr_new_instance
// ---------------
//
ProbeMod *
TvcProbe::pr_new_instance() {
    TvcProbe *pm = new TvcProbe("TVC test Inst", &tvc_probe_param, NULL);
    pm->mod_init(mod_params);
    pm->mod_startup();

    return pm;
}

// pr_intercept_request
// --------------------
//
void
TvcProbe::pr_intercept_request(ProbeRequest *req) {
}

// pr_put
// ------
//
void
TvcProbe::pr_put(ProbeRequest *probe) {
}

// pr_get
// ------
//
void
TvcProbe::pr_get(ProbeRequest *req) {
}

void
TvcProbe::startTx(const OpParams &startParams) {
    Error err = gl_DmTvcMod.startBlobTx(1, startParams.blobName,
            startParams.blobMode, startParams.txId);
    fds_verify(err == ERR_OK);
}

void
TvcProbe::updateTx(const OpParams &updateParams) {
    Error err = gl_DmTvcMod.updateBlobTx(1, updateParams.txId, updateParams.objList);
    fds_verify(err == ERR_OK);
}

void
TvcProbe::updateMetaTx(const OpParams &updateParams) {
    Error err = gl_DmTvcMod.updateBlobTx(1, updateParams.txId, updateParams.metaList);
    fds_verify(err == ERR_OK);
}

void
TvcProbe::commitTx(const OpParams &commitParams) {
    Error err = gl_DmTvcMod.commitBlobTx(1, commitParams.blobName,
            commitParams.txId, commitCallback);
    fds_verify(err == ERR_OK);
}

void
TvcProbe::abortTx(const OpParams &abortParams) {
    Error err = gl_DmTvcMod.abortBlobTx(1, abortParams.txId);
    fds_verify(err == ERR_OK);
}

// pr_delete
// ---------
//
void
TvcProbe::pr_delete(ProbeRequest *req) {
}

// pr_verify_request
// -----------------
//
void
TvcProbe::pr_verify_request(ProbeRequest *req) {
}

// pr_gen_report
// -------------
//
void
TvcProbe::pr_gen_report(std::string *out) {
}

// mod_init
// --------
//
int
TvcProbe::mod_init(SysParams const *const param) {
    Module::mod_init(param);
    return 0;
}

// mod_startup
// -----------
//
void
TvcProbe::mod_startup() {
    JsObjManager  *mgr;
    JsObjTemplate *svc;

    if (pr_parent != NULL) {
        mgr = pr_parent->pr_get_obj_mgr();
        svc = mgr->js_get_template("Run-Input");
        svc->js_register_template(new TvcWorkloadTemplate(mgr));
    }
}

// mod_shutdown
// ------------
//
void
TvcProbe::mod_shutdown() {
}

/**
 * Workload dispatcher
 */
JsObject *
TvcObjectOp::js_exec_obj(JsObject *parent,
                         JsObjTemplate *templ,
                         JsObjOutput *out) {
    fds_uint32_t numOps = parent->js_array_size();
    TvcObjectOp *node;
    TvcProbe::OpParams *info;

    // create volume with id 1
    VolumeDesc voldesc("volume", 1);
    voldesc.maxObjSizeInBytes = 4096;
    Error err = gl_DmTvcMod.addVolume(voldesc);
    fds_verify(err.ok());
    err = gl_DmTvcMod.activateVolume(1);
    fds_verify(err.ok());
    gl_DmTvcMod.queryIface()->registerExpungeObjectsCb(expungeObjects);

    for (fds_uint32_t i = 0; i < numOps; i++) {
        node = static_cast<TvcObjectOp *>((*parent)[i]);
        info = node->tvc_ops();
        std::cout << "Doing a " << info->op << " for blob "
                  << info->blobName << std::endl;

        if (info->op == "startTx") {
            gl_TvcProbe.startTx(*info);
        } else if (info->op == "updateTx") {
            gl_TvcProbe.updateTx(*info);
        } else if (info->op == "updateMetaTx") {
            gl_TvcProbe.updateMetaTx(*info);
        } else if (info->op == "commitTx") {
            gl_TvcProbe.commitTx(*info);
        } else if (info->op == "abortTx") {
            gl_TvcProbe.abortTx(*info);
        } else {
            fds_panic("Unknown operation %s!", info->op.c_str());
        }
    }

    return this;
}

}  // namespace fds
