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
        e2e_req_perf_ctx.type = AM_STAT_VOLUME_REQ;
        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }

    /// Status info to be returned
    fpi::VolumeStatus volumeStatus;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_STATVOLUMEREQ_H_
