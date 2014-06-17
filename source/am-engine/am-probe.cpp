/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <fds_process.h>
#include <am-engine/am-probe.h>
#include <fds-probe/s3-probe.h>
#include <am-engine/handlers/handlermappings.h>
#include <am-engine/handlers/responsehandler.h>

namespace fds {

static void
probeS3ServerEntry(ProbeS3Eng *s3Probe,
                   FDS_NativeAPI::ptr api) {
    // TODO(Andrew): Change the probe to take
    // a shared_ptr so we don't extract the raw ptr
    s3Probe->run_server(api.get());
}

AmProbe::AmProbe(const std::string &name,
                 probe_mod_param_t *param,
                 Module *owner)
        : ProbeMod(name.c_str(),
                   param,
                   owner),
          numThreads(0),
          numOps(0),
          recvdOps(0),
          numBuffers(0),
          bufSize(4096) {
}

AmProbe::~AmProbe() {
}

int
AmProbe::mod_init(SysParams const *const param) {
    FdsConfigAccessor conf(g_fdsprocess->get_conf_helper());
    numThreads = conf.get_abs<fds_uint32_t>("fds.am.testing.probe_num_threads");
    threadPool = fds_threadpoolPtr(new fds_threadpool(numThreads));

    txDesc.reset(new BlobTxId());
    putProps.reset(new PutProperties());
    numBuffers = 10;
    for (fds_uint32_t i = 0;
         i < numBuffers;
         i++) {
        char *buf = new char[bufSize];
        memset(buf, i, bufSize);
        dataBuffers.push_back(buf);
    }
    return 0;
}

void
AmProbe::mod_startup() {
    JsObjManager  *mgr;
    JsObjTemplate *svc;

    if (pr_parent != NULL) {
        mgr = pr_parent->pr_get_obj_mgr();
        svc = mgr->js_get_template("Run-Input");
        svc->js_register_template(new AmWorkloadTemplate(mgr));
    }
}

void
AmProbe::mod_shutdown() {
    listen_thread->join();
}

void
AmProbe::init_server(FDS_NativeAPI::ptr api) {
    LOGDEBUG << "Starting AM test probe...";
    am_api = api;

    listen_thread.reset(new boost::thread(&probeS3ServerEntry,
                                          &gl_probeS3Eng,
                                          am_api));
}

probe_mod_param_t Am_probe_param =
{
    .pr_stat_cnt     = 0,
    .pr_inj_pts_cnt  = 0,
    .pr_inj_act_cnt  = 0,
    .pr_max_sec_tout = 0
};

// pr_new_instance
// ---------------
//
ProbeMod *
AmProbe::pr_new_instance()
{
    AmProbe *apm = new AmProbe("Am Client Inst", &Am_probe_param, NULL);
    apm->mod_init(mod_params);
    apm->mod_startup();

    return apm;
}

// pr_intercept_request
// --------------------
//
void
AmProbe::pr_intercept_request(ProbeRequest *req)
{
}

// pr_put
// ------
//
void
AmProbe::pr_put(ProbeRequest *probe)
{
}

// pr_get
// ------
//
void
AmProbe::pr_get(ProbeRequest *req)
{
}

// pr_delete
// ---------
//
void
AmProbe::pr_delete(ProbeRequest *req)
{
}

// pr_verify_request
// -----------------
//
void
AmProbe::pr_verify_request(ProbeRequest *req)
{
}

// pr_gen_report
// -------------
//
void
AmProbe::pr_gen_report(std::string *out) {
}

struct ProbeStartBlobTxResponseHandler : public StartBlobTxResponseHandler {
    explicit ProbeStartBlobTxResponseHandler(apis::TxDescriptor &retVal)
            : StartBlobTxResponseHandler(retVal) {
        type = HandlerType::IMMEDIATE;
    }
    virtual ~ProbeStartBlobTxResponseHandler() {
    }
    typedef boost::shared_ptr<ProbeStartBlobTxResponseHandler> ptr;

    virtual void process() {
        gl_AmProbe.incResp();
    }
};

struct ProbeUpdateBlobResponseHandler : public PutObjectResponseHandler {
    explicit ProbeUpdateBlobResponseHandler() {
        type = HandlerType::IMMEDIATE;
    }
    virtual ~ProbeUpdateBlobResponseHandler() {
    }
    typedef boost::shared_ptr<ProbeUpdateBlobResponseHandler> ptr;

    virtual void process() {
        gl_AmProbe.incResp();
    }
};

int
probeUpdateBlobHandler(void *reqContext,
                       fds_uint64_t bufferSize,
                       fds_off_t offset,
                       char *buffer,
                       void *callbackData,
                       FDSN_Status status,
                       ErrorDetails* errDetails) {
    ProbeUpdateBlobResponseHandler* handler =
            reinterpret_cast<ProbeUpdateBlobResponseHandler*>(callbackData); //NOLINT
    handler->status = status;
    handler->errorDetails = errDetails;

    handler->call();

    delete handler;
    return 0;
}

void
AmProbe::incResp() {
    recvdOps++;

    if (recvdOps == numOps) {
        endTime = util::getTimeStampMicros();
        LOGCRITICAL << "Probe completed " << numOps << " ops in "
                    << endTime - startTime << " microsecs at a rate of "
                    << (1000 * 1000) * (static_cast<float>(numOps)
                                        / static_cast<float>(endTime - startTime));
        recvdOps = 0;
    }
    fds_verify(recvdOps <= numOps);
}

/**
 * Thread entry point to start a blob tx
 */
void
AmProbe::doAsyncStartTx(const std::string &volumeName,
                        const std::string &blobName) {
    apis::TxDescriptor *retVal = new apis::TxDescriptor();
    ProbeStartBlobTxResponseHandler::ptr handler(
        new ProbeStartBlobTxResponseHandler(*retVal));

    gl_AmProbe.am_api->StartBlobTx(volumeName, blobName, SHARED_DYN_CAST(Callback, handler));
}

/**
 * Thread entry point to update blob
 */
void
AmProbe::doAsyncUpdateBlob(const std::string &volumeName,
                           const std::string &blobName,
                           fds_uint64_t blobOffset,
                           fds_uint32_t dataLength,
                           const char *data) {
    BucketContext bucket_ctx("host", volumeName, "accessid", "secretkey");

    ProbeUpdateBlobResponseHandler *handler =
            new ProbeUpdateBlobResponseHandler();

    gl_AmProbe.am_api->PutBlob(&bucket_ctx,
                               blobName,
                               gl_AmProbe.putProps,
                               NULL,  // Not passing any context for the callback
                               gl_AmProbe.dataBuffers[0],  // Data buf
                               blobOffset,
                               dataLength,
                               gl_AmProbe.txDesc,
                               false,  // Never make it last
                               probeUpdateBlobHandler,
                               static_cast<void *>(handler));
}

JsObject *
AmProbe::AmProbeOp::js_exec_obj(JsObject *parent,
                                JsObjTemplate *templ,
                                JsObjOutput *out) {
    fds_uint32_t numOps = parent->js_array_size();
    AmProbeOp *node;
    AmProbe::OpParams *info;
    // Set the number of total ops were going to execute
    gl_AmProbe.numOps = numOps;
    gl_AmProbe.startTime = util::getTimeStampMicros();
    for (fds_uint32_t i = 0; i < numOps; i++) {
        node = static_cast<AmProbeOp *>((*parent)[i]);
        info = node->am_ops();
        LOGDEBUG << "Doing a " << info->op << " for blob "
                 << info->blobName;

        if (info->op == "startBlobTx") {
            gl_AmProbe.threadPool->schedule(AmProbe::doAsyncStartTx,
                                            info->volumeName,
                                            info->blobName);
        } else if (info->op == "updateBlob") {
            gl_AmProbe.threadPool->schedule(AmProbe::doAsyncUpdateBlob,
                                            info->volumeName,
                                            info->blobName,
                                            info->blobOffset,
                                            info->dataLength,
                                            info->data);
        } else if (info->op == "query") {
            // gl_Dm_ProbeMod.sendQuery(*info);
        }
    }

    return this;
}

AmProbe gl_AmProbe("Global Am test probe",
                   &Am_probe_param, NULL);

}  // namespace fds
