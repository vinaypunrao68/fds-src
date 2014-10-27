/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_REQUEST_COMMITBLOBTXREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_REQUEST_COMMITBLOBTXREQ_H_

#include <string>

#include "AmRequest.h"

namespace fds
{

struct CommitBlobTxReq : 
    public AmRequest,
    public AmTxReq
{
    /**
     * Request constructor. Some of the fields
     * are not actually needed...the base blob
     * request class just expects them.
     */
    CommitBlobTxReq(fds_volid_t        _volid,
                   const std::string &_vol_name,
                   const std::string &_blob_name,
                   BlobTxId::ptr _txDesc,
                   CallbackPtr        _cb) :
            AmRequest(FDS_COMMIT_BLOB_TX, _volid, _blob_name, 0, 0, 0, _cb),
            AmTxReq(_txDesc)
    {
        volume_name = _vol_name;
        e2e_req_perf_ctx.type = AM_COMMIT_BLOB_OBJ_REQ;
        e2e_req_perf_ctx.name = "volume:" + std::to_string(_volid);
        e2e_req_perf_ctx.reset_volid(_volid);

        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }
    virtual ~CommitBlobTxReq() {
        fds::PerfTracer::tracePointEnd(e2e_req_perf_ctx);
    }
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUEST_COMMITBLOBTXREQ_H_
