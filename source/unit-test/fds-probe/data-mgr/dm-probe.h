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
 * SM probe that acts as an SM client.
 * Provides implementation of JSON interface to
 * SM requests.
 */
class Dm_ProbeMod : public ProbeMod
{
  public:
    Dm_ProbeMod(char const *const name, probe_mod_param_t *param, Module *owner)
            : ProbeMod(name, param, owner),
              smIp("127.0.0.1"),
              myIp("127.0.0.1"),
              myName("DM probe client"),
              numChannels(1),
              numVols(1),
              numObjs(10),
              randData(false),
              verify(true),
              transId(0) {
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
        fds_uint64_t   blobOffset;
        blob_version_t blobVersion;
        ObjectID       objectId;  // TODO(Andrew): Make a list
        fds_uint32_t   objectLen;  // Also make a list
    };
    void sendUpdate(const OpParams &updateParams);
    void sendQuery(const OpParams &queryParams);

    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();

  private:
    void InitDmMsgHdr(const fdspi::FDSP_MsgHdrTypePtr& msg_hdr);

    std::string smIp; /**< Remote SM's IP */
    std::string myIp; /**< This client's IP */
    std::string myName;  /**< This client's name */
    fds_uint32_t numChannels;  /**< Number of channels per session*/
    std::string sessionId;  /**< UUID for client's session */
    fds_uint32_t transId;

    fds_uint32_t numVols;   /**< Number of objects to use */
    fds_uint32_t numObjs;   /**< Number of objects to use */
    fds_bool_t   randData;  /**< Use random data or not */
    fds_bool_t   verify;    /**< Verify data or not */
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
        Dm_ProbeMod::OpParams *p = new Dm_ProbeMod::OpParams();
        char *op;
        char *name;
        if (json_unpack(in, "{s:s, s:i, s:s, s:i}",
                        "blob-op", &op,
                        "volume-id", &p->volId,
                        "blob-name", &name,
                        "blob-off", &p->blobOffset)) {
            delete p;
            return NULL;
        }
        p->op = op;
        p->blobName = name;

        char *objectId;
        json_unpack(in, "{s:s}", "object-id", &objectId);
        p->objectId = objectId;

        json_unpack(in, "{s:i}", "object-len", &p->objectLen);

        json_unpack(in, "{s:i}", "blob-version", &p->blobVersion);

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
