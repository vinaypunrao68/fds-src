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
#include <fds_resource.h>
#include <fds_typedefs.h>

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

    std::cout << "Will add Catalog for volume " << name << std::hex
              << ":" << volParams.vol_id << std::dec << std::endl;
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
VolCatProbe::deleteVolume(const OpParams &volParams) {
    Error err = gl_DmVolCatMod.markVolumeDeleted(
        volParams.vol_id);
    std::cout << "Volume " << std::hex << volParams.vol_id
              << std::dec << " marked deleted " << err;
    if (err.ok()) {
      err = gl_DmVolCatMod.deleteEmptyCatalog(volParams.vol_id);
      fds_verify(err.ok());
    }
}

void VolCatProbe::getVolumeMeta(const OpParams& volParams) {
    Error err(ERR_OK);
    fds_uint64_t size = 0;
    fds_uint64_t blob_count = 0;
    fds_uint64_t object_count = 0;
    err = gl_DmVolCatMod.getVolumeMeta(volParams.vol_id, &size,
                                       &blob_count, &object_count);
    fds_verify(err.ok());
    std::cout << "Volume " << std::hex << volParams.vol_id
              << std::dec << " size " << size
              << " blob_count " << blob_count
              << " object_count " << object_count << std::endl;
}

void
VolCatProbe::listBlobs(const OpParams& volParams) {
    Error err(ERR_OK);
    fpi::BlobInfoListType blob_list;
    err = gl_DmVolCatMod.listBlobs(volParams.vol_id,
                                   &blob_list);
    fds_verify(err.ok() || (err == ERR_VOL_NOT_FOUND));
    std::cout << "List of blobs for volume " << std::hex
              << volParams.vol_id << std::dec << " " << err << std::endl;
    for (auto binfo : blob_list) {
        std::cout << "Blob " << binfo.blob_name
                  << " size " << binfo.blob_size << std::endl;
    }
}

void
VolCatProbe::getBlobMeta(const OpParams &getParams) {
    Error err(ERR_OK);
    fpi::FDSP_MetaDataList meta_list;
    blob_version_t blob_version = blob_version_invalid;
    fds_uint64_t blob_size = 0;

    err = gl_DmVolCatMod.getBlobMeta(getParams.vol_id,
                                     getParams.blob_name,
                                     &blob_version,
                                     &blob_size,
                                     &meta_list);
    fds_verify(err.ok() || (err == ERR_VOL_NOT_FOUND));
    MetaDataList mlist(meta_list);  // for easy printing
    std::cout << "Retrieved blob for volid " << std::hex
              << getParams.vol_id << std::dec << " blob name "
              << getParams.blob_name << " version " << blob_version
              << " blob size " << blob_size << " " << mlist
              << " " << err << std::endl;
}

void
VolCatProbe::getBlob(const OpParams& getParams) {
    Error err(ERR_OK);
    fpi::FDSP_MetaDataList meta_list;
    fpi::FDSP_BlobObjectList obj_list;
    blob_version_t blob_version = blob_version_invalid;

    std::set<fds_uint64_t> offsets;
    for (BlobObjList::const_iter cit = (getParams.obj_list)->cbegin();
         cit != (getParams.obj_list)->cend();
         ++cit) {
        offsets.insert(cit->first);
    }

    std::cout << "Will call getBlob for " << getParams.blob_name << std::endl;
    err = gl_DmVolCatMod.getBlob(getParams.vol_id,
                                 getParams.blob_name,
                                 *offsets.begin(),
                                 &blob_version,
                                 &meta_list, &obj_list);

    fds_verify(err.ok() || (err == ERR_VOL_NOT_FOUND));
    MetaDataList mlist(meta_list);  // for easy printing
    BlobObjList olist(obj_list);
    std::cout << "Retrieved blob for volid " << std::hex
              << getParams.vol_id << std::dec << " blob name "
              << getParams.blob_name << " version " << blob_version
              << " " << mlist << " " << olist << " " << err << std::endl;
}

void
VolCatProbe::putBlob(const OpParams &putParams) {
    Error err(ERR_OK);
    // must must have blob name!
    fds_verify(putParams.blob_name.size() > 0);

    BlobTxId::ptr tx_id(new BlobTxId(1));
    if ((putParams.obj_list)->size() == 0) {
        // no offset to object id list, use putBlobMeta
        std::cout << "Will call putBlobMeta for " << std::hex
                  << putParams.vol_id << std::dec << "," << putParams.blob_name
                  << "; " << *putParams.meta_list << std::endl;
        err = gl_DmVolCatMod.putBlobMeta(putParams.vol_id, putParams.blob_name,
                                         putParams.meta_list, tx_id);
        fds_verify(err.ok());
        return;
    }

    std::cout << "Will call putBlob for " << std::hex << putParams.vol_id
              << std::dec << "," << putParams.blob_name << ";"
              << *putParams.meta_list << "; "
              << *(putParams.obj_list) << std::endl;
    err = gl_DmVolCatMod.putBlob(putParams.vol_id, putParams.blob_name,
                                 putParams.meta_list, putParams.obj_list, tx_id);
    fds_verify(err.ok());
}

void
VolCatProbe::flushBlob(const OpParams &flushParams) {
}

void
VolCatProbe::deleteBlob(const OpParams &delParams) {
    Error err(ERR_OK);
    // must have blob name!
    fds_verify(delParams.blob_name.size() > 0);

    std::cout << "Will call deleteBlob for " << std::hex << delParams.vol_id
              << std::dec << "," << delParams.blob_name << std::endl;

    err = gl_DmVolCatMod.deleteBlob(delParams.vol_id, delParams.blob_name,
                                    blob_version_invalid);
    std::cout << "Delete blob result " << err << std::endl;
    fds_verify(err.ok() || err == ERR_BLOB_NOT_FOUND);
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
        } else if (info->op == "voldel") {
            gl_VolCatProbe.deleteVolume(*info);
        } else if (info->op == "volget") {
            gl_VolCatProbe.getVolumeMeta(*info);
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
        } else if (info->op == "list") {
            gl_VolCatProbe.listBlobs(*info);
        }
    }

    return this;
}

}  // namespace fds
