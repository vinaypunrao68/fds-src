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

DmPersistVolCatalog gl_DmVolCatPlMod("Global Unit test DM Volume Catalog Persistent Layer");

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
    std::cout << "Will create catalog for volume "
              << std::hex << volParams.vol_id << std::dec;
    std::string name = "vol" + std::to_string(volParams.vol_id);
    VolumeDesc voldesc(name, volParams.vol_id);
    voldesc.maxObjSizeInBytes = volParams.max_obj_size;
    err = gl_DmVolCatPlMod.createCatalog(voldesc);
    fds_verify(err.ok());
    err = gl_DmVolCatPlMod.openCatalog(volParams.vol_id);
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
        if (err.ok()) {
            std::cout << *bext << std::endl;
        }
    } else {
        std::cout << "Will call getExtent..." << std::endl;
        BlobExtent::ptr bext =
                gl_DmVolCatPlMod.getExtent(getParams.vol_id,
                                           getParams.blob_name,
                                           getParams.extent_id,
                                           err);
        if (err.ok()) {
            std::cout << *bext << std::endl;
        }
    }

    if (err == ERR_CAT_ENTRY_NOT_FOUND) {
        std::cout << "Extent not found for " << std::hex
                  << getParams.vol_id << std::dec << ","
                  << getParams.blob_name << ","
                  << getParams.extent_id << std::endl;
        return;
    }
    fds_verify(err.ok());
}

//
// over-writes extent with new data
//
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
            err = bext->updateOffset(cit->first, (cit->second).oid);
            fds_verify(err.ok());
        }

        std::cout << "Will call putExtent for " << *bext << std::endl;
        /*
        err = gl_DmVolCatPlMod.putExtent(putParams.vol_id,
                                         putParams.blob_name,
                                         putParams.extent_id,
                                         bext);
        */
        fds_verify(err.ok());
    } else {
        BlobExtent0::ptr bext(new BlobExtent0(putParams.blob_name,
                                              putParams.max_obj_size,
                                              putParams.first_offset,
                                              putParams.num_offsets));

        // since we are testing persistent layer, we don't care if
        // blob_version or blob size is set correctly, so we will just keep
        // default (zero) values for these

        // copy metadata list
        bext->updateMetaData(putParams.meta_list);

        // copy blob object list
        fds_uint64_t last_offset = 0;
        fds_uint64_t blob_size = 0;
        for (BlobObjList::const_iter cit = putParams.obj_list.cbegin();
             cit != putParams.obj_list.cend();
             ++cit) {
            err = bext->updateOffset(cit->first, (cit->second).oid);
            last_offset = cit->first;
            blob_size += (cit->second).size;
            fds_verify(err.ok());
        }
        bext->setLastBlobOffset(last_offset);

        std::cout << "Will call putExtentMeta for " << *bext << std::endl;
        err = gl_DmVolCatPlMod.putMetaExtent(putParams.vol_id,
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
    std::cout << "Will update extent " << putParams.extent_id
              << " for " << putParams.blob_name << std::endl;

    if (putParams.extent_id > 0) {
        BlobExtent::ptr bext =
                gl_DmVolCatPlMod.getExtent(putParams.vol_id,
                                           putParams.blob_name,
                                           putParams.extent_id,
                                           err);
        fds_verify(err.ok() || (err == ERR_CAT_ENTRY_NOT_FOUND));
        if (err.ok()) {
            std::cout << "Extent before update--  " << *bext << std::endl;
        } else if (err == ERR_CAT_ENTRY_NOT_FOUND) {
            std::cout << "Extent before update--  [none]" << std::endl;
        }

        // update blob object list
        for (BlobObjList::const_iter cit = putParams.obj_list.cbegin();
             cit != putParams.obj_list.cend();
             ++cit) {
            err = bext->updateOffset(cit->first, (cit->second).oid);
            fds_verify(err.ok());
        }

        std::cout << "Extent after update--  " << *bext << std::endl;
        /*
        err = gl_DmVolCatPlMod.putExtent(putParams.vol_id,
                                         putParams.blob_name,
                                         putParams.extent_id,
                                         bext);
        */
        fds_verify(err.ok());
    } else {
        BlobExtent0::ptr bext =
                gl_DmVolCatPlMod.getMetaExtent(putParams.vol_id,
                                               putParams.blob_name,
                                               err);
        fds_verify(err.ok() || (err == ERR_CAT_ENTRY_NOT_FOUND));
        if (err.ok()) {
            std::cout << "Extent before update--  " << *bext << std::endl;
        } else if (err == ERR_CAT_ENTRY_NOT_FOUND) {
            std::cout << "Extent before update--  [none]" << std::endl;
        }

        // update meta part of extent 0
        bext->updateMetaData(putParams.meta_list);

        // update blob object list
        fds_uint64_t blob_size = bext->blobSize();
        for (BlobObjList::const_iter cit = putParams.obj_list.cbegin();
             cit != putParams.obj_list.cend();
             ++cit) {
            ObjectID oid;
            err = bext->getObjectInfo(cit->first, &oid);
            if (err == ERR_NOT_FOUND) {
                // for now we are testing with const size obj sizes
                blob_size += (cit->second).size;
            }
            err = bext->updateOffset(cit->first, (cit->second).oid);
            fds_verify(err.ok());
        }
        bext->setBlobSize(blob_size);

        std::cout << "Extent after update--  " << *bext << std::endl;
        err = gl_DmVolCatPlMod.putMetaExtent(putParams.vol_id,
                                             putParams.blob_name,
                                             bext);
        fds_verify(err.ok());
    }
}


void
VolCatPlProbe::deleteExtent(const OpParams &delParams) {
    Error err(ERR_OK);
    // must have blob name!
    fds_verify(delParams.blob_name.size() > 0);
    std::cout << "Will delete extent " << delParams.extent_id
              << " for " << delParams.blob_name << std::endl;

    err = gl_DmVolCatPlMod.deleteExtent(delParams.vol_id,
                                        delParams.blob_name,
                                        delParams.extent_id);
    fds_verify(err.ok());
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
