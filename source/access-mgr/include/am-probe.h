/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AM_PROBE_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AM_PROBE_H_

#include <string>
#include <vector>
#include <fds_module.h>
#include <native_api.h>
#include <concurrency/ThreadPool.h>
#include <concurrency/RwLock.h>
#include <fds-probe/fds_probe.h>
#include <util/timeutils.h>

namespace fds {

/**
 * Class that provides interface mapping between
 * UBD, the user space block device, and AM.
 */
class AmProbe : public ProbeMod {
  private:
    FDS_NativeAPI::ptr               am_api;
    boost::shared_ptr<boost::thread> listen_thread;
    fds_bool_t                       testQos;
    fds_uint32_t                     numThreads;
    fds_uint32_t                     outstandingReqs;
    fds_uint32_t                     sleepPeriod;
    fds_uint32_t                     numOps;
    fds_atomic_ullong                recvdOps;
    fds_atomic_ullong                dispatchedOps;
    util::TimeStamp                  startTime;
    util::TimeStamp                  endTime;

    /// Test buffers to write
    // TODO(Andrew): Make this less stupid
    fds_uint32_t numBuffers;
    fds_uint32_t bufSize;
    BlobTxId::ptr txDesc;
    PutPropertiesPtr putProps;
    std::vector<char *> dataBuffers;

    // this is used only if testQos is true
    std::unordered_map<std::string, fds_uint32_t> volDispOps_;
    fds_rwlock dispLock_;

    /**
     * Parameters that can be set
     */
    class OpParams {
      public:
        std::string  op;
        std::string  volumeName;
        std::string  blobName;
        fds_uint64_t blobOffset;
        fds_uint32_t dataLength;
        char         *data;
    };

    /**
     * Operation service, type name "thpool-syscall",
     * "thpool-boost", "thpool-math"
     */
    class AmProbeOp : public JsObject {
      public:
        virtual JsObject *js_exec_obj(JsObject *array,
                                      JsObjTemplate *templ,
                                      JsObjOutput *out);
        inline AmProbe::OpParams *am_ops() {
            return static_cast<AmProbe::OpParams *>(js_pod_object());
        }
    };

    /**
     * Op service template for json templates
     */
    class AmOpTemplate : public JsObjTemplate {
      public:
        virtual ~AmOpTemplate() {}
        explicit AmOpTemplate(JsObjManager *mgr)
                : JsObjTemplate("am-ops", mgr) {
        }

        virtual JsObject *js_new(json_t *in) {
            AmProbe::OpParams *p = new AmProbe::OpParams();
            char *op;
            char *volName;
            char *blobName;
            if (json_unpack(in, "{s:s, s:s, s:s}",
                            "blob-op", &op,
                            "volume-name", &volName,
                            "blob-name", &blobName)) {
                delete p;
                return NULL;
            }
            p->op = op;
            p->volumeName = volName;
            p->blobName = blobName;

            json_unpack(in, "{s:i}", "blob-offset", &p->blobOffset);
            json_unpack(in, "{s:i}", "data-length", &p->dataLength);
            char *data;
            json_unpack(in, "{s:s}", "data", &data);
            p->data = data;

            return js_parse(new AmProbeOp(), in, p);
        }
    };

    class AmWorkloadTemplate : public JsObjTemplate {
      public:
        explicit AmWorkloadTemplate(JsObjManager *mgr)
                : JsObjTemplate("am-workload", mgr) {
            js_decode["am-ops"] = new AmOpTemplate(mgr);
        }
        virtual JsObject *js_new(json_t *in) {
            return js_parse(new JsObject(), in, NULL);
        }
    };

  public:
    explicit AmProbe(const std::string &name,
                     probe_mod_param_t *param,
                     Module *owner);
    virtual ~AmProbe();

    typedef boost::shared_ptr<AmProbe> ptr;

    // Module related methods
    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();
    void init_server(FDS_NativeAPI::ptr api);

    // Probe related methods
    ProbeMod *pr_new_instance();
    void pr_intercept_request(ProbeRequest *req);
    void pr_put(ProbeRequest *req);
    void pr_get(ProbeRequest *req);
    void pr_delete(ProbeRequest *req);
    void pr_verify_request(ProbeRequest *req);
    void pr_gen_report(std::string *out);

    // Threadpool for managing workload
    fds_threadpoolPtr threadPool;

    // Workload related methods
    void incResp();
    void incDispatched(const std::string& vol_name);
    void decDispatched(const std::string& vol_name);
    bool isDispatchedGreaterThan(const std::string& vol_name, uint64_t val);
    static void doAsyncStartTx(const std::string &volumeName,
                               const std::string &blobName,
                               const fds_int32_t blobMode);
    static void doAsyncUpdateBlob(const std::string &volumeName,
                                  const std::string &blobName,
                                  fds_uint64_t blobOffset,
                                  fds_uint32_t dataLength,
                                  const char *data);
    static void doAsyncGetBlob(const std::string &volumeName,
                               const std::string &blobName,
                               fds_uint64_t blobOffset,
                               fds_uint32_t dataLength);
};

extern AmProbe gl_AmProbe;

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AM_PROBE_H_
