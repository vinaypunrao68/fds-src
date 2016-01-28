/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_STARTBLOBTXREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_STARTBLOBTXREQ_H_

#include <string>

#include "AmRequest.h"

namespace fds
{

struct StartBlobTxReq : 
    public AmRequest,
    public AmTxReq
{
    fds_int32_t     blob_mode;

    AmRequest* delete_request { nullptr };

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
            AmRequest(FDS_START_BLOB_TX, _volid, _vol_name, _blob_name, _cb),
            blob_mode(_blob_mode)
    {
        e2e_req_perf_ctx.type = PerfEventType::AM_START_BLOB_OBJ_REQ;
        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_STARTBLOBTXREQ_H_
