/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_ABORTBLOBTXREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_ABORTBLOBTXREQ_H_

#include <string>

#include "AmRequest.h"

namespace fds
{

struct AbortBlobTxReq :
    public AmRequest,
    public AmTxReq
{
    /**
     * Request constructor. Some of the fields
     * are not actually needed...the base blob
     * request class just expects them.
     */
    AbortBlobTxReq(fds_volid_t          _volid,
                   const std::string&   _vol_name,
                   const std::string&   _blob_name,
                   BlobTxId::ptr        _txDesc,
                   CallbackPtr          _cb) :
        AmRequest(FDS_ABORT_BLOB_TX, _volid, _vol_name, _blob_name, _cb),
        AmTxReq(_txDesc)
    {
        e2e_req_perf_ctx.type = AM_ABORT_BLOB_OBJ_REQ;
        e2e_req_perf_ctx.name = "volume:" + std::to_string(_volid);
        e2e_req_perf_ctx.reset_volid(_volid);

        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }

    ~AbortBlobTxReq() { fds::PerfTracer::tracePointEnd(e2e_req_perf_ctx); }
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_ABORTBLOBTXREQ_H_
