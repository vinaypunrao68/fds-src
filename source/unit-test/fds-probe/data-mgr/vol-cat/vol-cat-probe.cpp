/*
 * Copyright 2014 Formation Data Systems, Inc.
 *
 * TVC workload probe adapter.
 */
#include <vol-cat-probe.h>
#include <string>
#include <list>
#include <set>
#include <vector>
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

DmVolumeCatalog gl_DmVolCatMod("Global Unit Test DM Volume Catalog");

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

Error
VolCatProbe::expungeObjects(fds_volid_t volid,
                            const std::vector<ObjectID>& oids) {
    Error err(ERR_OK);
    for (std::vector<ObjectID>::const_iterator cit = oids.cbegin();
         cit != oids.cend();
         ++cit) {
        std::cout << "Pretend expunging object " << *cit << std::endl;
    }
    return err;
}


void
VolCatProbe::addCatalog(const OpParams &volParams) {
    Error err(ERR_OK);

    gl_DmVolCatMod.registerExpungeObjectsCb(std::bind(
        &VolCatProbe::expungeObjects, this,
        std::placeholders::_1, std::placeholders::_2));

    std::string name = "vol" + std::to_string(volParams.vol_id);
    VolumeDesc voldesc(name, volParams.vol_id);
    voldesc.maxObjSizeInBytes = volParams.max_obj_size;

    std::cout << "Will add Catalog for volume " << name << std::endl;
    err = gl_DmVolCatMod.addCatalog(voldesc);
    fds_verify(err.ok());

    // also activate right away for now
    err = gl_DmVolCatMod.activateCatalog(volParams.vol_id);
    fds_verify(err.ok());
}

void
VolCatProbe::rmCatalog(const OpParams& volParams) {
    Error err(ERR_OK);
    err = gl_DmVolCatMod.removeVolumeMeta(volParams.vol_id);
    fds_verify(err.ok());
}

void
VolCatProbe::checkVolEmpty(const OpParams &volParams) {
    fds_bool_t bret = gl_DmVolCatMod.isVolumeEmpty(
        volParams.vol_id);
    std::cout << "Volume " << std::hex << volParams.vol_id
              << std::dec << " empty? " << bret;
}

void
VolCatProbe::listBlobs(const OpParams& volParams) {
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
VolCatProbe::getBlob(const OpParams& getParams) {
    Error err(ERR_OK);
    std::set<fds_uint64_t> offsets;
    for (BlobObjList::const_iter cit = (getParams.obj_list)->cbegin();
         cit != (getParams.obj_list)->cend();
         ++cit) {
        offsets.insert(cit->first);
    }

    std::cout << "Will call getBlobMeta..." << std::endl;
    BlobMetaDesc::const_ptr bmeta_desc =
            gl_DmVolCatMod.getBlobMeta(getParams.vol_id,
                                       getParams.blob_name,
                                       err);
    fds_verify(err.ok());
    std::cout << *bmeta_desc;

    // if getParams includes offsets, also get offset to oids
    if (offsets.size() > 0) {
        std::cout << "Will call getBlobObjects..." << std::endl;
        BlobObjList::const_ptr obj_list =
                gl_DmVolCatMod.getBlobObjects(getParams.vol_id,
                                              getParams.blob_name,
                                              offsets,
                                              err);
        fds_verify(err.ok());
        std::cout << *obj_list;
    }
}

void
VolCatProbe::putBlob(const OpParams &putParams) {
    Error err(ERR_OK);
    // must must have blob name!
    fds_verify(putParams.blob_name.size() > 0);

    BlobTxId::ptr tx_id(new BlobTxId(1));
    BlobMetaDesc::ptr bmeta_desc(new BlobMetaDesc());
    bmeta_desc->blob_name = putParams.blob_name;
    bmeta_desc->vol_id = putParams.vol_id;
    bmeta_desc->version = blob_version_invalid;
    bmeta_desc->blob_size = putParams.blob_size;

    // copy metadata list
    for (MetaDataList::const_iter cit = putParams.meta_list.cbegin();
         cit != putParams.meta_list.cend();
         ++cit) {
        (bmeta_desc->meta_list).updateMetaDataPair(cit->first, cit->second);
    }

    if ((putParams.obj_list)->size() == 0) {
        // no offset to object id list, use putBlobMeta
        std::cout << "Will call putBlobMeta for " << *bmeta_desc << std::endl;
        err = gl_DmVolCatMod.putBlobMeta(bmeta_desc, tx_id);
        fds_verify(err.ok());
        return;
    }

    std::cout << "Will call putBlob for " << *bmeta_desc
              << "; " << *(putParams.obj_list);
    err = gl_DmVolCatMod.putBlob(bmeta_desc, putParams.obj_list, tx_id);
    fds_verify(err.ok());
}

void
VolCatProbe::flushBlob(const OpParams &flushParams) {
}

void
VolCatProbe::deleteBlob(const OpParams &delParams) {
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

        if (info->op == "volcrt") {
            gl_VolCatProbe.addCatalog(*info);
        } else if (info->op == "put") {
            gl_VolCatProbe.putBlob(*info);
        } else if (info->op == "flush") {
            gl_VolCatProbe.flushBlob(*info);
        } else if (info->op == "get") {
            gl_VolCatProbe.getBlob(*info);
        } else if (info->op == "getMeta") {
            gl_VolCatProbe.getBlobMeta(*info);
        } else if (info->op == "delete") {
            gl_VolCatProbe.deleteBlob(*info);
        }
    }

    return this;
}

}  // namespace fds
