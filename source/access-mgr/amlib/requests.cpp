/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#include <map>
#include <string>

#include "requests/requests.h"
#include "AmCounters.h"
#include "fds_process.h"

namespace fds
{

std::unique_ptr<AMCounters> counters { nullptr };
std::once_flag counter_flag;

void init_counters()
{ counters.reset(new AMCounters("AM", g_fdsprocess->get_cntrs_mgr().get())); }

GetBlobReq::GetBlobReq(fds_volid_t _volid,
                       const std::string& _volumeName,
                       const std::string& _blob_name,  // same as objKey
                       CallbackPtr cb,
                       fds_uint64_t _blob_offset,
                       fds_uint64_t _data_len)
    : AmRequest(FDS_GET_BLOB, _volid, _volumeName, _blob_name, cb, _blob_offset, _data_len),
      get_metadata(false), oid_cached(false), metadata_cached(false) {
    stopwatch.start();

    qos_perf_ctx.type = PerfEventType::AM_GET_QOS;
    qos_perf_ctx.reset_volid(io_vol_id);
    hash_perf_ctx.type = PerfEventType::AM_GET_HASH;
    hash_perf_ctx.reset_volid(io_vol_id);
    dm_perf_ctx.type = PerfEventType::AM_GET_DM;
    dm_perf_ctx.reset_volid(io_vol_id);
    sm_perf_ctx.type = PerfEventType::AM_GET_SM;
    sm_perf_ctx.reset_volid(io_vol_id);

    e2e_req_perf_ctx.type = PerfEventType::AM_GET_OBJ_REQ;
    fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
}

GetBlobReq::~GetBlobReq()
{
    std::call_once(counter_flag, &init_counters);
    counters->gets_latency.update(stopwatch.getElapsedNanos());
}

PutBlobReq::PutBlobReq(fds_volid_t _volid,
                       const std::string& _volName,
                       const std::string& _blob_name,
                       fds_uint64_t _blob_offset,
                       fds_uint64_t _data_len,
                       boost::shared_ptr<std::string> _data,
                       BlobTxId::ptr _txDesc,
                       CallbackPtr _cb) :
AmRequest(FDS_PUT_BLOB, _volid, _volName, _blob_name, _cb, _blob_offset, _data_len),
    AmTxReq(_txDesc),
    resp_acks(2),
    ret_status(ERR_OK),
    final_meta_data(),
    final_blob_size(0ULL),
    dataPtr(_data)
{
    stopwatch.start();

    qos_perf_ctx.type = PerfEventType::AM_PUT_QOS;
    qos_perf_ctx.reset_volid(io_vol_id);
    hash_perf_ctx.type = PerfEventType::AM_PUT_HASH;
    hash_perf_ctx.reset_volid(io_vol_id);
    dm_perf_ctx.type = PerfEventType::AM_PUT_DM;
    dm_perf_ctx.reset_volid(io_vol_id);
    sm_perf_ctx.type = PerfEventType::AM_PUT_SM;
    sm_perf_ctx.reset_volid(io_vol_id);

    e2e_req_perf_ctx.type = PerfEventType::AM_PUT_OBJ_REQ;
    fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
}

PutBlobReq::PutBlobReq(fds_volid_t          _volid,
                       const std::string&   _volName,
                       const std::string&   _blob_name,
                       fds_uint64_t         _blob_offset,
                       fds_uint64_t         _data_len,
                       boost::shared_ptr<std::string> _data,
                       fds_int32_t          _blobMode,
                       boost::shared_ptr< std::map<std::string, std::string> >& _metadata,
                       CallbackPtr _cb) :
    AmRequest(FDS_PUT_BLOB_ONCE, _volid, _volName, _blob_name, _cb, _blob_offset, _data_len),
    blob_mode(_blobMode),
    metadata(_metadata),
    resp_acks(2),
    ret_status(ERR_OK),
    final_meta_data(),
    final_blob_size(0ULL),
    dataPtr(_data)
{
    stopwatch.start();

    qos_perf_ctx.type = PerfEventType::AM_PUT_QOS;
    qos_perf_ctx.reset_volid(io_vol_id);
    hash_perf_ctx.type = PerfEventType::AM_PUT_HASH;
    hash_perf_ctx.reset_volid(io_vol_id);
    dm_perf_ctx.type = PerfEventType::AM_PUT_DM;
    dm_perf_ctx.reset_volid(io_vol_id);
    sm_perf_ctx.type = PerfEventType::AM_PUT_SM;
    sm_perf_ctx.reset_volid(io_vol_id);

    e2e_req_perf_ctx.type = PerfEventType::AM_PUT_OBJ_REQ;
    fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
}

PutBlobReq::~PutBlobReq()
{
    std::call_once(counter_flag, &init_counters);
    counters->puts_latency.update(stopwatch.getElapsedNanos());
}

void
PutBlobReq::notifyResponse(const Error &e) {
    fds_verify(resp_acks > 0);
    if (0 == --resp_acks) {
        // Call back to processing layer
        proc_cb(e);
    }
}

}  // namespace fds
