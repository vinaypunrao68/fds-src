/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_UNIT_TEST_FDS_PROBE_DATA_MGR_VOL_CAT_VOL_CAT_PROBE_H_
#define SOURCE_UNIT_TEST_FDS_PROBE_DATA_MGR_VOL_CAT_VOL_CAT_PROBE_H_

/*
 * Header file for DM Volume Catalog probe
 */
#include <string>
#include <list>
#include <vector>
#include <fds-probe/fds_probe.h>
#include <utest-types.h>
#include <fds_types.h>
#include <dm-vol-cat/DmVolumeCatalog.h>

namespace fds {

    /**
     * Class that acts as unit test interface to DM Volume Catalog
     * Provides implementation of JSON interface to
     * DM Volume Catalog API requests.
     */
    class VolCatProbe : public ProbeMod {
  public:
        VolCatProbe(const std::string &name,
                    probe_mod_param_t *param,
                    Module *owner);
        virtual ~VolCatProbe();
        typedef boost::shared_ptr<VolCatProbe> ptr;

        // Module related methods
        int  mod_init(SysParams const *const param);
        void mod_startup();
        void mod_shutdown();

        // Probe related methods
        ProbeMod *pr_new_instance();
        void pr_intercept_request(ProbeRequest *req);
        void pr_put(ProbeRequest *req);
        void pr_get(ProbeRequest *req);
        void pr_delete(ProbeRequest *req);
        void pr_verify_request(ProbeRequest *req);
        void pr_gen_report(std::string *out);

        class OpParams {
      public:
            std::string    op;
            fds_volid_t    vol_id;
            fds_uint32_t max_obj_size;  // only used by vol ops
            std::string    blob_name;
            fds_uint64_t   blob_size;
            fds_bool_t     end_of_blob;
            MetaDataList::ptr   meta_list;
            BlobObjList::ptr    obj_list;

            OpParams()
                    : meta_list(new MetaDataList()),
                    obj_list(new BlobObjList()){
            }
        };
        void addCatalog(const OpParams &volParams);
        void rmCatalog(const OpParams& volParams);
        void getVolumeMeta(const OpParams& volParams);
        void deleteVolume(const OpParams &volParams);
        void listBlobs(const OpParams& volParams);
        void getBlobMeta(const OpParams &getParams);
        void getBlob(const OpParams &getParams);
        void putBlob(const OpParams &putParams);
        void flushBlob(const OpParams &flushParams);
        void deleteBlob(const OpParams &delParams);

        Error expungeObjects(fds_volid_t volid,
                             const std::vector<ObjectID>& oids);
};

// ----------------------------------------------------------------------------

/**
 * Unit test operation object
 */
class VolCatObjectOp : public JsObject
{
  public:
    virtual JsObject *js_exec_obj(JsObject *array,
                                  JsObjTemplate *templ,
                                  JsObjOutput *out);

    inline VolCatProbe::OpParams *volcat_ops() {
        return static_cast<VolCatProbe::OpParams *>(js_pod_object());
    }
};

/**
 * Unit test workload specification
 */
class VolCatOpTemplate : public JsObjTemplate
{
  public:
    virtual ~VolCatOpTemplate() {}
    explicit VolCatOpTemplate(JsObjManager *mgr)
            : JsObjTemplate("volcat-ops", mgr) {}

    virtual JsObject *js_new(json_t *in) {
        VolCatProbe::OpParams *p = new VolCatProbe::OpParams();
        char *op;
        char *name;
        char* meta_key;
        char* meta_val;
        p->vol_id = 0;
        if (json_unpack(in, "{s:s, s:i}",
                        "volcat-op", &op,
                        "volume-id", &p->vol_id)) {
            fds_panic("Malformed json!");
        }
        p->op = op;
        p->blob_name = "";
        p->max_obj_size = 4096;
        p->end_of_blob = false;

        if (!json_unpack(in, "{s:i}",
                         "max-obj-size", &p->max_obj_size)) {
            fds_verify(p->max_obj_size > 0);
        }
        if ((p->op == "volcrt") || (p->op == "list") ||
            (p->op == "volchk")) {
            return js_parse(new VolCatObjectOp(), in, p);
        }

        // blob name
        if (!json_unpack(in, "{ s:s}",
                         "blob-name", &name)) {
            p->blob_name = name;
        }

        // end of blob
        if (!json_unpack(in, "{s:i}",
                         "blob-end", &p->end_of_blob)) {
        }

        // we will ensure that blob-name specified for all blob
        // operations when we execute blob operations

        (p->meta_list)->clear();
        json_t *meta;
        if (!json_unpack(in, "{s:o}",
                        "metadata", &meta)) {
            for (fds_uint32_t i = 0; i < json_array_size(meta); ++i) {
                std::string meta_pair = json_string_value(json_array_get(meta, i));
                std::size_t k = meta_pair.find_first_of("-");
                fds_verify(k != std::string::npos);  // must separate key-value with "-"
                std::string meta_key = meta_pair.substr(0, k);
                std::string meta_val = meta_pair.substr(k+1);
                (p->meta_list)->updateMetaDataPair(meta_key, meta_val);
            }
        }

        // read object list
        (p->obj_list)->clear();
        p->blob_size = 0;
        json_t* objlist;
        ObjectID oid;
        if (!json_unpack(in, "{s:o}",
                        "objects", &objlist)) {
            for (fds_uint32_t i = 0; i < json_array_size(objlist); ++i) {
                std::string offset_obj = json_string_value(json_array_get(objlist, i));
                std::size_t k = offset_obj.find_first_of("-");
                fds_verify(k != std::string::npos);  // must separate key-value with "-"
                std::string offset_str = offset_obj.substr(0, k);
                std::string objinfo_str = offset_obj.substr(k+1);
                k = objinfo_str.find_first_of("-");
                fds_verify(k != std::string::npos);  // must separate key-value with "-"
                std::string oid_hexstr = objinfo_str.substr(0, k);
                std::string size_str = objinfo_str.substr(k+1);
                fds_uint64_t offset = stoull(offset_str);
                fds_uint64_t size = stoull(size_str);
                oid = oid_hexstr;
                (p->obj_list)->updateObject(offset, oid, size);
                p->blob_size += size;
            }
            if (p->end_of_blob > 0) {
                (p->obj_list)->setEndOfBlob();
            }
        }

        return js_parse(new VolCatObjectOp(), in, p);
    }
};

/**
 * Unit test mapping for json structure
 */
class VolCatWorkloadTemplate : public JsObjTemplate
{
  public:
    explicit VolCatWorkloadTemplate(JsObjManager *mgr)
            : JsObjTemplate("volcat-workload", mgr)
    {
        js_decode["volcat-ops"] = new VolCatOpTemplate(mgr);
    }
    virtual JsObject *js_new(json_t *in) {
        return js_parse(new JsObject(), in, NULL);
    }
};

/// Adapter class
extern VolCatProbe           gl_VolCatProbe;
extern DmVolumeCatalog       gl_DmVolCatMod;

}  // namespace fds

#endif  // SOURCE_UNIT_TEST_FDS_PROBE_DATA_MGR_VOL_CAT_VOL_CAT_PROBE_H_
