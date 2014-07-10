/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_UNIT_TEST_FDS_PROBE_DATA_MGR_TVC_COMMIT_LOG_COMMIT_LOG_PROBE_H_
#define SOURCE_UNIT_TEST_FDS_PROBE_DATA_MGR_TVC_COMMIT_LOG_COMMIT_LOG_PROBE_H_

/*
 * Header file template for probe adapter.  Replace Thrpool with your probe name
 */
#include <string>
#include <list>
#include <fds-probe/fds_probe.h>
#include <utest-types.h>
#include <fds_types.h>
#include <dm-tvc/CommitLog.h>

namespace fds {

/**
 * Class that acts as unit test interface to CommitLog.
 * Provides implementation of JSON interface to
 * CommitLog API requests.
 */
class CommitLogProbe : public ProbeMod {
  public:
    CommitLogProbe(const std::string &name,
             probe_mod_param_t *param,
             Module *owner);
    virtual ~CommitLogProbe();
    typedef boost::shared_ptr<CommitLogProbe> ptr;

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
        std::string              op;
        fds_volid_t              volId;
        BlobTxId::ptr            txId;
        std::string              blobName;
        fpi::FDSP_MetaDataList   metaList;
        fpi::FDSP_BlobObjectList objList;
    };
    void startTx(const OpParams &startParams);
    void updateTx(const OpParams &updateParams);
    void updateMetaTx(const OpParams &updateParams);
    void commitTx(const OpParams &commitParams);
    void abortTx(const OpParams &abortParams);
    void purgeTx(const OpParams & purgeParams);
};

// ----------------------------------------------------------------------------

/**
 * Unit test operation object
 */
class CommitLogObjectOp : public JsObject
{
  public:
    virtual JsObject *js_exec_obj(JsObject *array,
                                  JsObjTemplate *templ,
                                  JsObjOutput *out);

    inline CommitLogProbe::OpParams *commit_log_ops() {
        return static_cast<CommitLogProbe::OpParams *>(js_pod_object());
    }
};

/**
 * Unit test workload specification
 */
class CommitLogOpTemplate : public JsObjTemplate
{
  public:
    virtual ~CommitLogOpTemplate() {}
    explicit CommitLogOpTemplate(JsObjManager *mgr)
            : JsObjTemplate("commit-log-ops", mgr) {}

    virtual JsObject *js_new(json_t *in) {
        CommitLogProbe::OpParams *p = new CommitLogProbe::OpParams();
        char *op;
        char *name;
        fds_uint32_t txId;
        if (json_unpack(in, "{s:s, s:i, s:s}",
                        "blob-op", &op,
                        "tx-id", &txId,
                        "blob-name", &name)) {
            fds_panic("Malformed json!");
        }
        p->op = op;
        p->txId = BlobTxId::ptr(new BlobTxId(txId));
        p->blobName = name;

        // json_unpack(in, "{s:i}", "blob-off", &p->blobOffset);

        // char *objectId;
        // json_unpack(in, "{s:s}", "object-id", &objectId);
        // p->objectId = objectId;
        // p->objectId = NULL;

        return js_parse(new CommitLogObjectOp(), in, p);
    }
};

/**
 * Unit test mapping for json structure
 */
class CommitLogWorkloadTemplate : public JsObjTemplate
{
  public:
    explicit CommitLogWorkloadTemplate(JsObjManager *mgr)
            : JsObjTemplate("commit-log-workload", mgr)
    {
        js_decode["commit-log-ops"] = new CommitLogOpTemplate(mgr);
    }
    virtual JsObject *js_new(json_t *in) {
        return js_parse(new JsObject(), in, NULL);
    }
};

/// Adapter class
extern CommitLogProbe gl_CommitLogProbe;

// Commmit Log
extern DmCommitLog gl_DmCommitLogMod;

}  // namespace fds

#endif  // SOURCE_UNIT_TEST_FDS_PROBE_DATA_MGR_TVC_COMMIT_LOG_COMMIT_LOG_PROBE_H_
