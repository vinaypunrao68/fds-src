/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_GETBLOBMETADATAREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_GETBLOBMETADATAREQ_H_

#include <string>

#include "FdsBlobReq.h"

namespace fds
{

struct GetBlobMetaDataReq : FdsBlobReq {
    GetBlobMetaDataReq(fds_volid_t volId, const std::string & volumeName,
                       const std::string &_blob_name, CallbackPtr cb) :
            FdsBlobReq(FDS_GET_BLOB_METADATA, volId, _blob_name , 0, 0, NULL, cb) {
        volume_name = volumeName;
        e2e_req_perf_ctx.type = AM_GET_BLOB_META_OBJ_REQ;
        e2e_req_perf_ctx.name = "volume:" + std::to_string(volId);
        e2e_req_perf_ctx.reset_volid(volId);

        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }

    virtual ~GetBlobMetaDataReq() {
        fds::PerfTracer::tracePointEnd(e2e_req_perf_ctx);
    }
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_GETBLOBMETADATAREQ_H_
