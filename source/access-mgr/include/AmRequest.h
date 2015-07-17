/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMREQUEST_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMREQUEST_H_

#include <string>

#include "fds_volume.h"
#include "PerfTrace.h"
#include "responsehandler.h"

namespace fds
{

class AmRequest : public FDS_IOType {
    // Callback members
    typedef std::function<void (const Error&)> ProcessorCallback;

 public:
    // Performance
    PerfContext    e2e_req_perf_ctx;
    PerfContext    qos_perf_ctx;
    PerfContext    hash_perf_ctx;
    PerfContext    dm_perf_ctx;
    PerfContext    sm_perf_ctx;

    std::size_t    data_len;
    fds_uint64_t   blob_offset;
    fds_uint64_t   blob_offset_end;
    std::string    volume_name;

    // Flag to indicate when a request has been responded to
    std::atomic<bool> completed;

    ProcessorCallback proc_cb;
    CallbackPtr cb;

    AmRequest(fds_io_op_t         _op,
              fds_volid_t         _vol_id,
              const std::string&  _vol_name,
              const std::string&  _blob_name,
              CallbackPtr         _cb,
              fds_uint64_t        _blob_offset = 0,
              fds_uint64_t        _data_len = 0)
        : volume_name(_vol_name),
        completed(false),
        blob_name(_blob_name),
        blob_offset(_blob_offset),
        data_len(_data_len),
        cb(_cb)
    {
        io_module = ACCESS_MGR_IO;
        io_req_id = 0;
        io_type   = _op;
        setVolId(_vol_id);
    }

    void setVolId(fds_volid_t const vol_id) {
        io_vol_id = vol_id;
        e2e_req_perf_ctx.reset_volid(io_vol_id);
        qos_perf_ctx.reset_volid(io_vol_id);
        hash_perf_ctx.reset_volid(io_vol_id);
        dm_perf_ctx.reset_volid(io_vol_id);
        sm_perf_ctx.reset_volid(io_vol_id);
    }

    virtual ~AmRequest()
    { fds::PerfTracer::tracePointEnd(e2e_req_perf_ctx); }

    bool isCompleted()
    { return !completed.load(std::memory_order_relaxed); }

    bool testAndSetComplete()
    { return completed.exchange(true, std::memory_order_relaxed); }

    const std::string& getBlobName() const
    { return blob_name; }

 protected:
    std::string        blob_name;
};

struct AmTxReq {
    AmTxReq() : tx_desc(nullptr) {}
    explicit AmTxReq(BlobTxId::ptr _tx_desc) : tx_desc(_tx_desc) {}
    BlobTxId::ptr tx_desc;
    fds_uint64_t dmt_version;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMREQUEST_H_
