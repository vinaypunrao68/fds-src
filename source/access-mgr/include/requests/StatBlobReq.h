/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_STATBLOBREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_STATBLOBREQ_H_

#include <string>

#include "AmRequest.h"

namespace fds
{

class StatBlobReq : public AmRequest {
  public:
    /**
     * Request constructor. Some of the fields
     * are not actually needed...the base blob
     * request class just expects them.
     */
    StatBlobReq(fds_volid_t          _volid,
                const std::string   &_vol_name,
                const std::string   &_blob_name,
                CallbackPtr         cb) :
            AmRequest(FDS_STAT_BLOB, _volid, _vol_name, _blob_name, cb) {
        e2e_req_perf_ctx.type = AM_STAT_BLOB_OBJ_REQ;
        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_STATBLOBREQ_H_
