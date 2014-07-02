/*
 * Copyright 2014 Formation Data Systems, Inc.
 *
 * TVC workload probe adapter.
 */
#include <vol-cat-probe.h>
#include <string>
#include <list>
#include <utest-types.h>

namespace fds {

probe_mod_param_t volcat_probe_param =
{
    .pr_stat_cnt     = 0,
    .pr_inj_pts_cnt  = 0,
    .pr_inj_act_cnt  = 0,
    .pr_max_sec_tout = 0
};

/// Global singleton probe module
VolCatProbe gl_VolCatProbe("Volume Catalog Probe Adapter",
                     &volcat_probe_param,
                     nullptr);

VolCatProbe::VolCatProbe(const std::string &name,
                         probe_mod_param_t *param,
                         Module *owner)
        : ProbeMod(name.c_str(),
                   param,
                   owner) {
}

VolCatProbe::~VolCatProbe() {
}

// pr_new_instance
// ---------------
//
ProbeMod *
VolCatProbe::pr_new_instance() {
    VolCatProbe *pm = new VolCatProbe("Volume Catalog test Inst", &volcat_probe_param, NULL);
    pm->mod_init(mod_params);
    pm->mod_startup();

    return pm;
}

// pr_intercept_request
// --------------------
//
void
VolCatProbe::pr_intercept_request(ProbeRequest *req) {
}

// pr_put
// ------
//
void
VolCatProbe::pr_put(ProbeRequest *probe) {
}

void
VolCatProbe::getBlobMeta(const OpParams &getParams) {
    Error err(ERR_OK);
    BlobMetaDesc::const_ptr bmeta_desc =
            gl_DmVolCatMod.getBlobMeta(getParams.vol_id,
                                       getParams.blob_name,
                                       err);
    fds_verify(err.ok());
    std::cout << *bmeta_desc;
}

void
VolCatProbe::putBlobMeta(const OpParams&putParams) {
    Error err(ERR_OK);
    BlobMetaDesc::ptr bmeta_desc(new BlobMetaDesc());
    bmeta_desc->blob_name = putParams.blob_name;
    bmeta_desc->vol_id = putParams.vol_id;
    bmeta_desc->version = blob_version_invalid;
    bmeta_desc->blob_size = 0;

    // copy metadata list
    for (MetaDataList::const_iter cit = putParams.meta_list.cbegin();
         cit != putParams.meta_list.cend();
         ++cit) {
        (bmeta_desc->meta_list).updateMetaDataPair(cit->first, cit->second);
    }
    std::cout << "Will call putBlobMeta for " << *bmeta_desc;
    err = gl_DmVolCatMod.putBlobMeta(bmeta_desc);
    fds_verify(err.ok());
}

void
VolCatProbe::putBlob(const OpParams &putParams) {
    Error err(ERR_OK);
    BlobMetaDesc::ptr bmeta_desc(new BlobMetaDesc());
    bmeta_desc->blob_name = putParams.blob_name;
    bmeta_desc->vol_id = putParams.vol_id;
    bmeta_desc->version = blob_version_invalid;
    bmeta_desc->blob_size = 0;

    // copy metadata list
    for (MetaDataList::const_iter cit = putParams.meta_list.cbegin();
         cit != putParams.meta_list.cend();
         ++cit) {
        (bmeta_desc->meta_list).updateMetaDataPair(cit->first, cit->second);
    }

    // create blob object list
    BlobObjList::ptr obj_list(new BlobObjList());
    for (BlobObjList::const_iter cit = putParams.obj_list.cbegin();
         cit != putParams.obj_list.cend();
         ++cit) {
        obj_list->updateObject(cit->first, cit->second);
    }

    std::cout << "Will call putBlob for " << *bmeta_desc
              << "; " << *obj_list;

    err = gl_DmVolCatMod.putBlob(bmeta_desc, obj_list);
    fds_verify(err.ok());
}

// pr_get
// ------
//
void
VolCatProbe::pr_get(ProbeRequest *req) {
}


// pr_delete
// ---------
//
void
VolCatProbe::pr_delete(ProbeRequest *req) {
}

// pr_verify_request
// -----------------
//
void
VolCatProbe::pr_verify_request(ProbeRequest *req) {
}

// pr_gen_report
// -------------
//
void
VolCatProbe::pr_gen_report(std::string *out) {
}

// mod_init
// --------
//
int
VolCatProbe::mod_init(SysParams const *const param) {
    Module::mod_init(param);
    return 0;
}

// mod_startup
// -----------
//
void
VolCatProbe::mod_startup() {
    JsObjManager  *mgr;
    JsObjTemplate *svc;

    if (pr_parent != NULL) {
        mgr = pr_parent->pr_get_obj_mgr();
        svc = mgr->js_get_template("Run-Input");
        svc->js_register_template(new VolCatWorkloadTemplate(mgr));
    }
}

// mod_shutdown
// ------------
//
void
VolCatProbe::mod_shutdown() {
}

/**
 * Workload dispatcher
 */
JsObject *
VolCatObjectOp::js_exec_obj(JsObject *parent,
                            JsObjTemplate *templ,
                            JsObjOutput *out) {
    fds_uint32_t numOps = parent->js_array_size();
    VolCatObjectOp *node;
    VolCatProbe::OpParams *info;
    for (fds_uint32_t i = 0; i < numOps; i++) {
        node = static_cast<VolCatObjectOp *>((*parent)[i]);
        info = node->volcat_ops();
        std::cout << "Doing a " << info->op << " for blob "
                  << info->blob_name << std::endl;

        if (info->op == "putMeta") {
            gl_VolCatProbe.putBlobMeta(*info);
        } else if (info->op == "putBlob") {
            gl_VolCatProbe.putBlob(*info);
        }
    }

    return this;
}

}  // namespace fds
