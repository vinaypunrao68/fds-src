/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMREQUEST_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMREQUEST_H_

#include <string>

#include "native/types.h"

namespace fds
{

struct AmRequest {
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

     /*
      * Callback members
      * TODO: Resolve this with what's needed by the object-based callbacks.
      */
     typedef boost::function<void(fds_int32_t)> callbackBind;
     typedef std::function<void (const Error&)> ProcessorCallback;

     ProcessorCallback proc_cb;

     AmRequest(fds_io_op_t         _op,
                fds_volid_t         _volId,
                const std::string&  _blobName,
                fds_uint64_t        _blobOffset,
                fds_uint64_t        _dataLen,
                char*               _dataBuf);

     AmRequest(fds_io_op_t         _op,
                fds_volid_t         _volId,
                const std::string&  _blobName,
                fds_uint64_t        _blobOffset,
                fds_uint64_t        _dataLen,
                char*               _dataBuf,
                CallbackPtr         _cb);

     template<typename F, typename A, typename B, typename C>
        AmRequest(fds_io_op_t          _op,
                   fds_volid_t          _volId,
                   const std::string&   _blobName,
                   fds_uint64_t         _blobOffset,
                   fds_uint64_t         _dataLen,
                   char*                _dataBuf,
                   F f, A a, B b, C c);

     virtual ~AmRequest()
     { magic = FDS_SH_IO_MAGIC_NOT_IN_USE; }

     CallbackPtr cb;

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

     void setDataBuf(const char* _buf)
     { memcpy(data_buf, _buf, data_len); }

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

inline AmRequest::AmRequest(fds_io_op_t      _op,
                       fds_volid_t        _volId,
                       const std::string &_blobName,
                       fds_uint64_t       _blobOffset,
                       fds_uint64_t       _dataLen,
                       char              *_dataBuf)
        : magic(FDS_SH_IO_MAGIC_IN_USE),
          io_type(_op),
          vol_id(_volId),
          blob_name(_blobName),
          blob_offset(_blobOffset),
          data_len(_dataLen),
          data_buf(_dataBuf)
{ }

inline AmRequest::AmRequest(fds_io_op_t       _op,
                       fds_volid_t        _volId,
                       const std::string &_blobName,
                       fds_uint64_t       _blobOffset,
                       fds_uint64_t       _dataLen,
                       char              *_dataBuf,
                       CallbackPtr        _cb)
        : magic(FDS_SH_IO_MAGIC_IN_USE),
          io_type(_op),
          vol_id(_volId),
          blob_name(_blobName),
          blob_offset(_blobOffset),
          data_len(_dataLen),
          data_buf(_dataBuf),
          cb(_cb)
{ }

template<typename F, typename A, typename B, typename C>
inline AmRequest::AmRequest(fds_io_op_t      _op,
           fds_volid_t        _volId,
           const std::string &_blobName,
           fds_uint64_t       _blobOffset,
           fds_uint64_t       _dataLen,
           char              *_dataBuf,
           F f,
           A a,
           B b,
           C c)
    : magic(FDS_SH_IO_MAGIC_IN_USE),
    io_type(_op),
    vol_id(_volId),
    blob_name(_blobName),
    blob_offset(_blobOffset),
    data_len(_dataLen),
    data_buf(_dataBuf),
    callback(boost::bind(f, a, b, c, _1))
{ }

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMREQUEST_H_
