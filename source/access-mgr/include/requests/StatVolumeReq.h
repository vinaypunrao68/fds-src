/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_STATVOLUMEREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_STATVOLUMEREQ_H_

#include <string>

#include "AmRequest.h"

namespace fds
{

struct StatVolumeReq : AmRequest {
    StatVolumeReq(fds_volid_t volId, const std::string & volumeName, CallbackPtr cb) :
            AmRequest(FDS_STAT_VOLUME, volId, volumeName, "", cb) {
        e2e_req_perf_ctx.type = PerfEventType::AM_STAT_VOLUME_REQ;
        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }

    size_t blob_count {0};
    size_t size {0};
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_STATVOLUMEREQ_H_
