/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_UNIT_TEST_FDS_PROBE_DATA_MGR_VOL_CAT_PL_VOLCAT_PL_PROBE_H_
#define SOURCE_UNIT_TEST_FDS_PROBE_DATA_MGR_VOL_CAT_PL_VOLCAT_PL_PROBE_H_

/*
 * Header file for DM Volume Catalog probe
 */
#include <string>
#include <list>
#include <fds-probe/fds_probe.h>
#include <utest-types.h>
#include <fds_types.h>
#include <dm-vol-cat/DmPersistVc.h>

namespace fds {

    // NOTE!!! for now this is hardcoded to match consts in vol cat
    // persistent layer, make sure they match
    static const fds_uint32_t probe_extent0_obj_entries = 1024;
    static const fds_uint32_t probe_extent_obj_entries = 2048;

    /**
     * Class that acts as unit test interface to DM Volume Catalog
     * persistent layer
     * Provides implementation of JSON interface to
     * DM Volume Catalog Persistent Layer API requests.
     */
    class VolCatPlProbe : public ProbeMod {
  public:
        VolCatPlProbe(const std::string &name,
                      probe_mod_param_t *param,
                      Module *owner);
        virtual ~VolCatPlProbe();
        typedef boost::shared_ptr<VolCatPlProbe> ptr;

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
            fds_extent_id  extent_id;
            fds_uint32_t   max_obj_size;  // in bytes
            fds_uint32_t   first_offset;  // in max_obj_size units
            fds_uint32_t   num_offsets;
            std::string    blob_name;
            MetaDataList::ptr   meta_list;  // empty for non-zero extents
            BlobObjList    obj_list;   // offsets are in bytes
            OpParams()
                    : meta_list(new MetaDataList()) {
            }
        };
        void createCatalog(const OpParams &volParams);
        void getExtent(const OpParams &getParams);
        void putExtent(const OpParams &putParams);  // over-writes extent if exists
        void updateExtent(const OpParams &putParams);  // read-modify-write
        void deleteExtent(const OpParams &delParams);
};

// ----------------------------------------------------------------------------

/**
 * Unit test operation object
 */
class VolCatPlObjectOp : public JsObject
{
  public:
    virtual JsObject *js_exec_obj(JsObject *array,
                                  JsObjTemplate *templ,
                                  JsObjOutput *out);

    inline VolCatPlProbe::OpParams *volcat_ops() {
        return static_cast<VolCatPlProbe::OpParams *>(js_pod_object());
    }
};

/**
 * Unit test workload specification
 */
class VolCatPlOpTemplate : public JsObjTemplate
{
  public:
    virtual ~VolCatPlOpTemplate() {}
    explicit VolCatPlOpTemplate(JsObjManager *mgr)
            : JsObjTemplate("volcat-pl-ops", mgr) {}

    virtual JsObject *js_new(json_t *in) {
        VolCatPlProbe::OpParams *p = new VolCatPlProbe::OpParams();
        char *op;
        char *name;
        char* meta_key;
        char* meta_val;
        fds_uint32_t first_offset = 0;
        fds_uint32_t last_offset = 0;
        if (json_unpack(in, "{s:s, s:i}",
                        "volcat-op", &op,
                        "volume-id", &p->vol_id)) {
            fds_panic("Malformed json!");
        }
        p->op = op;
        p->blob_name = "";
        p->extent_id = 0;
        p->max_obj_size = 4096;

        if (p->op == "volcrt") {
            return js_parse(new VolCatPlObjectOp(), in, p);
        }

        if (!json_unpack(in, "{s:s, s:i, s:i}",
                         "blob-name", &name,
                         "extent-id", &p->extent_id,
                         "max-obj-size", &p->max_obj_size)) {
            p->blob_name = name;
        }
        // we will ensure that blob-name specified for all blob
        // operations when we execute blob operations
        fds_verify(p->max_obj_size > 0);
        if (p->extent_id > 0) {
            first_offset =
                    probe_extent0_obj_entries +
                    (p->extent_id-1) * probe_extent_obj_entries;
            last_offset =
                    probe_extent0_obj_entries +
                    p->extent_id * probe_extent_obj_entries - 1;
            p->num_offsets = probe_extent_obj_entries;
        } else if (p->extent_id == 0) {
            first_offset = 0;
            last_offset = probe_extent0_obj_entries - 1;
            p->num_offsets = probe_extent0_obj_entries;
        }
        p->first_offset = first_offset;

        (p->meta_list)->clear();
        json_t *meta;
        if (!json_unpack(in, "{s:o}",
                        "metadata", &meta)) {
            fds_verify(p->op != "volcrt");
            fds_verify(p->extent_id == 0);  // metadata only for extent 0
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
        (p->obj_list).clear();
        json_t* objlist;
        ObjectID oid;
        if (!json_unpack(in, "{s:o}",
                        "objects", &objlist)) {
            fds_verify(p->op != "volcrt");
            for (fds_uint32_t i = 0; i < json_array_size(objlist); ++i) {
                std::string offset_obj = json_string_value(json_array_get(objlist, i));
                std::size_t k = offset_obj.find_first_of("-");
                fds_verify(k != std::string::npos);  // must separate key-value with "-"
                std::string offset_str = offset_obj.substr(0, k);
                std::string oid_hexstr = offset_obj.substr(k+1);
                fds_uint32_t offset = stoul(offset_str);
                oid = oid_hexstr;
                offset += first_offset;  // offset in json is relative to extent first offset
                // offset must be within range allowed for this extent
                // offset is in units of max obj size (not bytes!)
                fds_verify((offset >= first_offset) &&
                           (offset <= last_offset));
                fds_uint64_t offset_bytes = offset * p->max_obj_size;
                (p->obj_list).updateObject(offset_bytes, oid, p->max_obj_size);
            }
        }

        return js_parse(new VolCatPlObjectOp(), in, p);
    }
};

/**
 * Unit test mapping for json structure
 */
class VolCatPlWorkloadTemplate : public JsObjTemplate
{
  public:
    explicit VolCatPlWorkloadTemplate(JsObjManager *mgr)
            : JsObjTemplate("volcat-pl-workload", mgr)
    {
        js_decode["volcat-pl-ops"] = new VolCatPlOpTemplate(mgr);
    }
    virtual JsObject *js_new(json_t *in) {
        return js_parse(new JsObject(), in, NULL);
    }
};

/// Adapter class
extern VolCatPlProbe           gl_VolCatPlProbe;
extern DmPersistVolCatalog     gl_DmVolCatPlMod;

}  // namespace fds

#endif  // SOURCE_UNIT_TEST_FDS_PROBE_DATA_MGR_VOL_CAT_PL_VOLCAT_PL_PROBE_H_
