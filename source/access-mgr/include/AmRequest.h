/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMREQUEST_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMREQUEST_H_

#include <string>

#include "native/types.h"

namespace fds
{

class AmRequest {
    // Callback members
    typedef boost::function<void(fds_int32_t)> callbackBind;
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
    fds_volid_t    vol_id;
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
              fds_uint64_t        _data_len = 0,
              char*               _data_buf = nullptr)
        : magic(FDS_SH_IO_MAGIC_IN_USE),
        io_type(_op),
        vol_id(_vol_id),
        volume_name(_vol_name),
        blob_name(_blob_name),
        blob_offset(_blob_offset),
        data_len(_data_len),
        data_buf(_data_buf),
        cb(_cb)
    {
        e2e_req_perf_ctx.name = "volume:" + std::to_string(_vol_id);
        e2e_req_perf_ctx.reset_volid(_vol_id);
    }

    template<typename F, typename A, typename B, typename C>
    AmRequest(fds_io_op_t          _op,
              fds_volid_t          _vol_id,
              const std::string&   _vol_name,
              const std::string&   _blob_name,
              fds_uint64_t         _blob_offset,
              fds_uint64_t         _data_len,
              char*                _data_buf,
              F f, A a, B b, C c)
        : magic(FDS_SH_IO_MAGIC_IN_USE),
        io_type(_op),
        vol_id(_vol_id),
        blob_name(_blob_name),
        blob_offset(_blob_offset),
        data_len(_data_len),
        data_buf(_data_buf),
        callback(boost::bind(f, a, b, c, _1))
    {
        e2e_req_perf_ctx.name = "volume:" + std::to_string(_vol_id);
        e2e_req_perf_ctx.reset_volid(_vol_id);
    }

    virtual ~AmRequest()
    {
        fds::PerfTracer::tracePointEnd(e2e_req_perf_ctx);
        magic = FDS_SH_IO_MAGIC_NOT_IN_USE;
    }

    bool magicInUse() const
    { return (magic == FDS_SH_IO_MAGIC_IN_USE); }

    fds_io_op_t  getIoType() const
    { return io_type; }

    void cbWithResult(int result)
    { return callback(result); }

    const std::string& getBlobName() const
    { return blob_name; }

    const char *getDataBuf() const
    { return data_buf; }

    void setQueuedUsec(fds_uint64_t _usec)
    { queued_usec = _usec; }

 protected:
    fds_uint32_t       magic;
    fds_io_op_t        io_type;
    fds_uint64_t       queued_usec;  /* Time spent in queue */
    char*              data_buf;
    std::string        blob_name;
    callbackBind       callback;
    util::StopWatch    stopwatch;
};

struct AmTxReq {
    AmTxReq() : tx_desc(nullptr) {}
    explicit AmTxReq(BlobTxId::ptr _tx_desc) : tx_desc(_tx_desc) {}
    BlobTxId::ptr tx_desc;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMREQUEST_H_
