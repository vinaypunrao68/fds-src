/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMREQUEST_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMREQUEST_H_

#include <string>

#include "native/types.h"

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
    std::string    volume_name;
    ObjectID       obj_id;

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
        blob_name(_blob_name),
        blob_offset(_blob_offset),
        data_len(_data_len),
        cb(_cb)
    {
        io_magic  = FDS_SH_IO_MAGIC_IN_USE;
        io_module = STOR_HV_IO;
        io_req_id = 0;
        io_type   = _op;
        io_vol_id = _vol_id;

        e2e_req_perf_ctx.name = "volume:" + std::to_string(_vol_id);
        e2e_req_perf_ctx.reset_volid(_vol_id);
    }

    virtual ~AmRequest()
    { fds::PerfTracer::tracePointEnd(e2e_req_perf_ctx); }

    bool magicInUse() const
    { return (io_magic == FDS_SH_IO_MAGIC_IN_USE); }

    const std::string& getBlobName() const
    { return blob_name; }

 protected:
    std::string        blob_name;
    util::StopWatch    stopwatch;
};

struct AmTxReq {
    AmTxReq() : tx_desc(nullptr) {}
    explicit AmTxReq(BlobTxId::ptr _tx_desc) : tx_desc(_tx_desc) {}
    BlobTxId::ptr tx_desc;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMREQUEST_H_
