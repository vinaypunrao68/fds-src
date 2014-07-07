/*
 * Copyright 2014 Formation Data Systems, Inc.
 *
 * TVC workload probe adapter.
 */
#include <volcat-pl-probe.h>
#include <string>
#include <list>
#include <set>
#include <utest-types.h>

namespace fds {

probe_mod_param_t volcat_pl_probe_param =
{
    .pr_stat_cnt     = 0,
    .pr_inj_pts_cnt  = 0,
    .pr_inj_act_cnt  = 0,
    .pr_max_sec_tout = 0
};

/// Global singleton probe module
VolCatPlProbe gl_VolCatPlProbe("Volume Catalog Persistent Layer Probe Adapter",
                               &volcat_pl_probe_param,
                               nullptr);

VolCatPlProbe::VolCatPlProbe(const std::string &name,
                             probe_mod_param_t *param,
                             Module *owner)
        : ProbeMod(name.c_str(),
                   param,
                   owner) {
}

VolCatPlProbe::~VolCatPlProbe() {
}

// pr_new_instance
// ---------------
//
ProbeMod *
VolCatPlProbe::pr_new_instance() {
    VolCatPlProbe *pm = new VolCatPlProbe("Volume Catalog Persistent Layer test Inst",
                                          &volcat_pl_probe_param, NULL);
    pm->mod_init(mod_params);
    pm->mod_startup();

    return pm;
}

// pr_intercept_request
// --------------------
//
void
VolCatPlProbe::pr_intercept_request(ProbeRequest *req) {
}

// pr_put
// ------
//
void
VolCatPlProbe::pr_put(ProbeRequest *probe) {
}

void
VolCatPlProbe::createCatalog(const OpParams &volParams) {
    Error err(ERR_OK);
    std::string name = "vol" + std::to_string(volParams.vol_id);
    VolumeDesc voldesc(name, volParams.vol_id);
    err = gl_DmVolCatPlMod.createCatalog(voldesc);
    fds_verify(err.ok());
}

void
VolCatPlProbe::getExtent(const OpParams& getParams) {
    Error err(ERR_OK);
    if (getParams.extent_id == 0) {
        std::cout << "Will call getMetaExtent..." << std::endl;
        BlobExtent0::ptr bext =
                gl_DmVolCatPlMod.getMetaExtent(getParams.vol_id,
                                               getParams.blob_name,
                                               err);
        fds_verify(err.ok());
        std::cout << *bext;
    } else {
        std::cout << "Will call getExtent..." << std::endl;
        BlobExtent::ptr bext =
                gl_DmVolCatPlMod.getExtent(getParams.vol_id,
                                           getParams.blob_name,
                                           getParams.extent_id,
                                           err);
        fds_verify(err.ok());
        std::cout << *bext;
    }
}

void
VolCatPlProbe::putExtent(const OpParams &putParams) {
    Error err(ERR_OK);
    // must have blob name!
    fds_verify(putParams.blob_name.size() > 0);

    if (putParams.extent_id > 0) {
        BlobExtent::ptr bext(new BlobExtent(putParams.extent_id,
                                            putParams.max_obj_size,
                                            putParams.first_offset,
                                            putParams.num_offsets));
        // copy blob object list
        for (BlobObjList::const_iter cit = putParams.obj_list.cbegin();
             cit != putParams.obj_list.cend();
             ++cit) {
            bext->updateOffset(cit->first, cit->second);
        }

        std::cout << "Will call putExtent for " << *bext;
        gl_DmVolCatPlMod.putExtent(putParams.vol_id,
                                   putParams.blob_name,
                                   putParams.extent_id,
                                   bext);
        fds_verify(err.ok());
    } else {
        BlobExtent0::ptr bext(new BlobExtent0(putParams.blob_name,
                                              putParams.vol_id,
                                              putParams.max_obj_size,
                                              putParams.first_offset,
                                              putParams.num_offsets));

        // copy metadata list
        for (MetaDataList::const_iter cit = putParams.meta_list.cbegin();
             cit != putParams.meta_list.cend();
             ++cit) {
            // TODO(xxx) add metadata
        }

        // copy blob object list
        for (BlobObjList::const_iter cit = putParams.obj_list.cbegin();
             cit != putParams.obj_list.cend();
             ++cit) {
            bext->updateOffset(cit->first, cit->second);
        }

        std::cout << "Will call putExtentMeta for " << *bext;
        gl_DmVolCatPlMod.putMetaExtent(putParams.vol_id,
                                       putParams.blob_name,
                                       bext);
        fds_verify(err.ok());
    }
}

//
// Reads extent from persistent layer if it exists, modifies it
// and writes updates extent back to persistent layer
//
void
VolCatPlProbe::updateExtent(const OpParams &putParams) {
    Error err(ERR_OK);
    // must have blob name!
    fds_verify(putParams.blob_name.size() > 0);
    if (putParams.extent_id > 0) {
        std::cout << "Will update extent "
                  << putParams.extent_id << std::endl;
        fds_verify(err.ok());
    } else {
        fds_verify(err.ok());
    }
}


void
VolCatPlProbe::deleteExtent(const OpParams &delParams) {
}

// pr_get
// ------
//
void
VolCatPlProbe::pr_get(ProbeRequest *req) {
}


// pr_delete
// ---------
//
void
VolCatPlProbe::pr_delete(ProbeRequest *req) {
}

// pr_verify_request
// -----------------
//
void
VolCatPlProbe::pr_verify_request(ProbeRequest *req) {
}

// pr_gen_report
// -------------
//
void
VolCatPlProbe::pr_gen_report(std::string *out) {
}

// mod_init
// --------
//
int
VolCatPlProbe::mod_init(SysParams const *const param) {
    Module::mod_init(param);
    return 0;
}

// mod_startup
// -----------
//
void
VolCatPlProbe::mod_startup() {
    JsObjManager  *mgr;
    JsObjTemplate *svc;

    if (pr_parent != NULL) {
        mgr = pr_parent->pr_get_obj_mgr();
        svc = mgr->js_get_template("Run-Input");
        svc->js_register_template(new VolCatPlWorkloadTemplate(mgr));
    }
}

// mod_shutdown
// ------------
//
void
VolCatPlProbe::mod_shutdown() {
}

/**
 * Workload dispatcher
 */
JsObject *
VolCatPlObjectOp::js_exec_obj(JsObject *parent,
                              JsObjTemplate *templ,
                              JsObjOutput *out) {
    fds_uint32_t numOps = parent->js_array_size();
    VolCatPlObjectOp *node;
    VolCatPlProbe::OpParams *info;
    for (fds_uint32_t i = 0; i < numOps; i++) {
        node = static_cast<VolCatPlObjectOp *>((*parent)[i]);
        info = node->volcat_ops();
        std::cout << "Doing a " << info->op << " for blob "
                  << info->blob_name << std::endl;

        if (info->op == "volcrt") {
            gl_VolCatPlProbe.createCatalog(*info);
        } else if (info->op == "put") {
            gl_VolCatPlProbe.putExtent(*info);
        } else if (info->op == "update") {
            gl_VolCatPlProbe.updateExtent(*info);
        } else if (info->op == "get") {
            gl_VolCatPlProbe.getExtent(*info);
        } else if (info->op == "delete") {
            gl_VolCatPlProbe.deleteExtent(*info);
        }
    }

    return this;
}

}  // namespace fds
