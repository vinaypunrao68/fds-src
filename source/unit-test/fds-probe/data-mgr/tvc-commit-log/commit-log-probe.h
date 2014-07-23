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
#include <fdsp/FDSP_types.h>
#include <fds-probe/fds_probe.h>
#include <utest-types.h>
#include <fds_types.h>
#include <fdsp_utils.h>
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
        using FDS_ProtocolInterface::FDS_ObjectIdType;
        using FDS_ProtocolInterface::FDSP_BlobObjectInfo;
        using FDS_ProtocolInterface::FDSP_MetaDataPair;

        CommitLogProbe::OpParams *p = new CommitLogProbe::OpParams();
        fds_uint32_t txId;

        const json_t * jsonOp = json_object_get(in, "blob-op");
        fds_verify(jsonOp);

        p->op = json_string_value(jsonOp);
        fds_verify(!p->op.empty());

        const json_t * jsonTxId = json_object_get(in, "tx-id");
        fds_verify(jsonTxId);
        fds_verify(json_is_number(jsonTxId));
        txId = json_integer_value(jsonTxId);

        p->txId = BlobTxId::ptr(new BlobTxId(txId));

        if ("abortTx" != p->op && "commitTx" != p->op && "purgeTx" != p->op) {
            if ("startTx" == p->op) {
                const json_t * jsonName = json_object_get(in, "blob-name");
                fds_verify(jsonName);
                p->blobName = json_string_value(jsonName);
                fds_verify(!p->blobName.empty());
            } else if ("updateTx" == p->op) {
                json_t * jsonObjList = json_object_get(in, "obj-list");
                fds_verify(jsonObjList);

                FDS_ObjectIdType objId = strToObjectIdType(json_string_value(
                            json_object_get(jsonObjList, "obj-id")));

                FDSP_BlobObjectInfo objInfo;
                objInfo.__set_offset(json_integer_value(json_object_get(jsonObjList, "offset")));
                objInfo.__set_size(json_integer_value(json_object_get(jsonObjList, "length")));
                objInfo.__set_data_obj_id(objId);
                p->objList.push_back(objInfo);
            } else if ("updateMetaTx" == p->op) {
                json_t * jsonMetaData = json_object_get(in, "meta-data");
                fds_verify(jsonMetaData);

                for (void * iter = json_object_iter(jsonMetaData); NULL != iter;
                        iter = json_object_iter_next(jsonMetaData, iter)) {
                    std::string keyStr = json_object_iter_key(iter);
                    std::string valStr = json_string_value(json_object_iter_value(iter));

                    FDSP_MetaDataPair metaData;
                    metaData.__set_key(keyStr);
                    metaData.__set_value(valStr);

                    p->metaList.push_back(metaData);
                }
            }
        }

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
