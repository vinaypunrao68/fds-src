/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_DELETEBLOBREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_DELETEBLOBREQ_H_

#include <string>

#include "native_api.h"
#include "AmRequest.h"

namespace fds
{

struct DeleteBlobReq: AmRequest {
    BucketContext *bucket_ctxt;
    std::string ObjKey;
    void *req_context;
    fdsnResponseHandler responseCallback;
    void *callback_data;
    BlobTxId::ptr tx_desc;

    fds_volid_t base_vol_id;

    DeleteBlobReq(fds_volid_t volId,
                  const std::string& _blob_name,
                  BucketContext* _bucket_ctxt,
                  void* _req_context,
                  fdsnResponseHandler _resp_handler,
                  void* _callback_data)
            : AmRequest(FDS_DELETE_BLOB, volId, _blob_name, 0, 0, NULL,
                         FDS_NativeAPI::DoCallback, this, Error(ERR_OK), 0),
              bucket_ctxt(_bucket_ctxt),
              ObjKey(_blob_name),
              req_context(_req_context),
              responseCallback(_resp_handler),
              callback_data(_callback_data) {
        e2e_req_perf_ctx.type = AM_DELETE_OBJ_REQ;
        e2e_req_perf_ctx.name = "volume:" + std::to_string(volId);
        e2e_req_perf_ctx.reset_volid(volId);
        qos_perf_ctx.type = AM_DELETE_QOS;
        qos_perf_ctx.name = "volume:" + std::to_string(volId);
        qos_perf_ctx.reset_volid(volId);
        hash_perf_ctx.type = AM_DELETE_HASH;
        hash_perf_ctx.name = "volume:" + std::to_string(volId);
        hash_perf_ctx.reset_volid(volId);
        dm_perf_ctx.type = AM_DELETE_DM;
        dm_perf_ctx.name = "volume:" + std::to_string(volId);
        dm_perf_ctx.reset_volid(volId);
        sm_perf_ctx.type = AM_DELETE_SM;
        sm_perf_ctx.name = "volume:" + std::to_string(volId);
        sm_perf_ctx.reset_volid(volId);

        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }

    DeleteBlobReq(fds_volid_t volId,
                  const std::string& _blob_name,
                  const std::string& volumeName,
                  CallbackPtr cb)
            : AmRequest(FDS_DELETE_BLOB, volId,
                         _blob_name, 0, 0, NULL, cb) {
        volume_name = volumeName;

        e2e_req_perf_ctx.type = AM_DELETE_OBJ_REQ;
        e2e_req_perf_ctx.name = "volume:" + std::to_string(volId);
        e2e_req_perf_ctx.reset_volid(volId);
        qos_perf_ctx.type = AM_DELETE_QOS;
        qos_perf_ctx.name = "volume:" + std::to_string(volId);
        qos_perf_ctx.reset_volid(volId);
        hash_perf_ctx.type = AM_DELETE_HASH;
        hash_perf_ctx.name = "volume:" + std::to_string(volId);
        hash_perf_ctx.reset_volid(volId);
        dm_perf_ctx.type = AM_DELETE_DM;
        dm_perf_ctx.name = "volume:" + std::to_string(volId);
        dm_perf_ctx.reset_volid(volId);
        sm_perf_ctx.type = AM_DELETE_SM;
        sm_perf_ctx.name = "volume:" + std::to_string(volId);
        sm_perf_ctx.reset_volid(volId);

        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }

    virtual ~DeleteBlobReq() {
        fds::PerfTracer::tracePointEnd(e2e_req_perf_ctx);
    }

    void DoCallback(FDSN_Status status, ErrorDetails* errDetails) {
        (responseCallback)(status, errDetails, callback_data);
    }
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_DELETEBLOBREQ_H_
