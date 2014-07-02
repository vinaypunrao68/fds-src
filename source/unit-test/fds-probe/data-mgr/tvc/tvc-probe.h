/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_UNIT_TEST_FDS_PROBE_DATA_MGR_TVC_TVC_PROBE_H_
#define SOURCE_UNIT_TEST_FDS_PROBE_DATA_MGR_TVC_TVC_PROBE_H_

/*
 * Header file template for probe adapter.  Replace Thrpool with your probe name
 */
#include <string>
#include <list>
#include <fds-probe/fds_probe.h>
#include <utest-types.h>
#include <fds_types.h>
#include <dm-tvc/TimeVolumeCatalog.h>

namespace fds {

/**
 * Class that acts as unit test interface to TVC.
 * Provides implementation of JSON interface to
 * TVC API requests.
 */
class TvcProbe : public ProbeMod {
  public:
    TvcProbe(const std::string &name,
             probe_mod_param_t *param,
             Module *owner);
    virtual ~TvcProbe();
    typedef boost::shared_ptr<TvcProbe> ptr;

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
        fds_volid_t    volId;
        BlobTxId::ptr  txId;
        std::string    blobName;
        fds_uint64_t   blobOffset;
        ObjectID       objectId;  // TODO(Andrew): Make a list
    };
    void startTx(const OpParams &startParams);
    void commitTx(const OpParams &commitParams);
};

// ----------------------------------------------------------------------------

/**
 * Unit test operation object
 */
class TvcObjectOp : public JsObject
{
  public:
    virtual JsObject *js_exec_obj(JsObject *array,
                                  JsObjTemplate *templ,
                                  JsObjOutput *out);

    inline TvcProbe::OpParams *tvc_ops() {
        return static_cast<TvcProbe::OpParams *>(js_pod_object());
    }
};

/**
 * Unit test workload specification
 */
class TvcOpTemplate : public JsObjTemplate
{
  public:
    virtual ~TvcOpTemplate() {}
    explicit TvcOpTemplate(JsObjManager *mgr)
            : JsObjTemplate("tvc-ops", mgr) {}

    virtual JsObject *js_new(json_t *in) {
        TvcProbe::OpParams *p = new TvcProbe::OpParams();
        char *op;
        char *name;
        fds_uint32_t txId;
        if (json_unpack(in, "{s:s, s:i, s:i, s:s}",
                        "blob-op", &op,
                        "volume-id", &p->volId,
                        "tx-id", &txId,
                        "blob-name", &name)) {
            fds_panic("Malformed json!");
        }
        p->op = op;
        p->txId = BlobTxId::ptr(new BlobTxId(txId));
        p->blobName = name;

        json_unpack(in, "{s:i}", "blob-off", &p->blobOffset);

        // char *objectId;
        // json_unpack(in, "{s:s}", "object-id", &objectId);
        // p->objectId = objectId;
        // p->objectId = NULL;

        return js_parse(new TvcObjectOp(), in, p);
    }
};

/**
 * Unit test mapping for json structure
 */
class TvcWorkloadTemplate : public JsObjTemplate
{
  public:
    explicit TvcWorkloadTemplate(JsObjManager *mgr)
            : JsObjTemplate("tvc-workload", mgr)
    {
        js_decode["tvc-ops"] = new TvcOpTemplate(mgr);
    }
    virtual JsObject *js_new(json_t *in) {
        return js_parse(new JsObject(), in, NULL);
    }
};

/// Adapter class
extern TvcProbe           gl_TvcProbe;

}  // namespace fds

#endif  // SOURCE_UNIT_TEST_FDS_PROBE_DATA_MGR_TVC_TVC_PROBE_H_
