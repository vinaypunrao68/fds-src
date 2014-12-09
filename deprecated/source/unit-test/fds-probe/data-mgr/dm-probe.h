/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_UNIT_TEST_FDS_PROBE_DATA_MGR_DM_PROBE_H_
#define SOURCE_UNIT_TEST_FDS_PROBE_DATA_MGR_DM_PROBE_H_

/*
 * Header file template for probe adapter.  Replace Thrpool with your probe name
 */
#include <string>
#include <list>
#include <fds-probe/fds_probe.h>
#include <utest-types.h>
#include <fds_types.h>
#include <NetSession.h>
#include <DataMgr.h>

namespace fds {

class probeDataPathRespIf;

namespace fdspi = FDS_ProtocolInterface;

extern std::unordered_map<ObjectID, std::string, ObjectHash> writtenObjs;
extern fds_mutex objMapLock;

/**
 * DM probe that acts as an SM client.
 * Provides implementation of JSON interface to
 * SM requests.
 */
class Dm_ProbeMod : public ProbeMod
{
  public:
    Dm_ProbeMod(char const *const name, probe_mod_param_t *param, Module *owner)
            : ProbeMod(name, param, owner) {
    }
    virtual ~Dm_ProbeMod() {
    }

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
        fds_volid_t    volId;
        std::string    blobName;
        fds_uint64_t   blobSize;
        blob_version_t blobVersion;
        fds_uint64_t   txId;
        MetaDataList   meta_list;
        BlobObjList::ptr   obj_list;
        OpParams()
                : obj_list(new BlobObjList()) {
        }
        fds_int32_t        blobMode;
    };

    void genericResp(EPSvcRequest* svcReq,
                     const Error& error,
                     boost::shared_ptr<std::string> payload);
    void schedule(const OpParams &updateParams);
    void sendUpdate(const OpParams &updateParams);
    void sendQuery(const OpParams &queryParams);
    void sendDelete(const OpParams &deleteParams);
    void sendStartTx(const OpParams &deleteParams);
    void sendCommitTx(const OpParams &deleteParams);
    void sendAbortTx(const OpParams &deleteParams);
    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();

  private:
    fpi::SvcUuid dm_uuid;
};

// ----------------------------------------------------------------------------

/**
 * Unit test "workload"
 */
class UT_ObjectOp : public JsObject
{
  public:
    virtual JsObject *js_exec_obj(JsObject *array,
                                  JsObjTemplate *templ,
                                  JsObjOutput *out);

    inline Dm_ProbeMod::OpParams *dm_ops() {
        return static_cast<Dm_ProbeMod::OpParams *>(js_pod_object());
    }
};

class UT_DmOpTemplate : public JsObjTemplate
{
  public:
    virtual ~UT_DmOpTemplate() {}
    explicit UT_DmOpTemplate(JsObjManager *mgr)
            : JsObjTemplate("dm-ops", mgr) {}

    virtual JsObject *js_new(json_t *in) {
        std::cout << " Unpacking ops" << std::endl;
        Dm_ProbeMod::OpParams *p = new Dm_ProbeMod::OpParams();
        char *op;
        char *name;
        if (!json_unpack(in, "{s:s, s:i, s:s, s:i, s:i, s:b}",
                        "blob-op", &op,
                        "volume-id", &p->volId,
                        "blob-name", &name,
                        "blob-ver", &p->blobVersion,
                        "blob-txId", &p->txId,
                        "blob-end", &p->blobMode)) {
            delete p;
            return NULL;
        }
        p->op = op;
        p->blobName = name;

        (p->meta_list).clear();
        json_t *meta;
        if (!json_unpack(in, "{s:o}",
                        "metadata", &meta)) {
            for (fds_uint32_t i = 0; i < json_array_size(meta); ++i) {
                std::string meta_pair = json_string_value(json_array_get(meta, i));
                std::size_t k = meta_pair.find_first_of("-");
                fds_verify(k != std::string::npos);  // must separate key-value with "-"
                std::string meta_key = meta_pair.substr(0, k);
                std::string meta_val = meta_pair.substr(k+1);
                (p->meta_list).updateMetaDataPair(meta_key, meta_val);
            }
        }

       // read object list
        (p->obj_list)->clear();
        p->blobSize = 0;
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
                p->blobSize += size;
            }
        }

        return js_parse(new UT_ObjectOp(), in, p);
    }
};

class UT_DmWorkloadTemplate : public JsObjTemplate
{
  public:
    explicit UT_DmWorkloadTemplate(JsObjManager *mgr)
            : JsObjTemplate("dm-workload", mgr)
    {
        js_decode["dm-ops"] = new UT_DmOpTemplate(mgr);
    }
    virtual JsObject *js_new(json_t *in) {
        return js_parse(new JsObject(), in, NULL);
    }
};

// Adapter class
extern Dm_ProbeMod           gl_Dm_ProbeMod;

}  // namespace fds

#endif  // SOURCE_UNIT_TEST_FDS_PROBE_DATA_MGR_DM_PROBE_H_
