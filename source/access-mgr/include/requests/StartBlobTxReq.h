/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_STARTBLOBTXREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_STARTBLOBTXREQ_H_

#include <string>

#include "FdsBlobReq.h"

namespace fds
{

class StartBlobTxReq : public FdsBlobReq {
  public:
    std::string     volumeName;
    fds_int32_t     blobMode;
    BlobTxId::ptr   tx_desc;
    fds_uint64_t    dmtVersion;

    typedef std::function<void (const Error&)> StartBlobTxProcCb;
    StartBlobTxProcCb processorCb;

    /**
     * Request constructor. Some of the fields
     * are not actually needed...the base blob
     * request class just expects them.
     */
    StartBlobTxReq(fds_volid_t        _volid,
                   const std::string &_vol_name,
                   const std::string &_blob_name,
                   const int32_t _blob_mode,
                   CallbackPtr        _cb) :
            FdsBlobReq(FDS_START_BLOB_TX, _volid, _blob_name, 0, 0, 0, _cb),
            volumeName(_vol_name), blobMode(_blob_mode) {
        volume_name = _vol_name;
        e2e_req_perf_ctx.type = AM_START_BLOB_OBJ_REQ;
        e2e_req_perf_ctx.name = "volume:" + std::to_string(_volid);
        e2e_req_perf_ctx.reset_volid(_volid);

        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }

    virtual ~StartBlobTxReq() {
        fds::PerfTracer::tracePointEnd(e2e_req_perf_ctx);
    }

    fds_int32_t getBlobMode() const {
        return blobMode;
    }
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_STARTBLOBTXREQ_H_
