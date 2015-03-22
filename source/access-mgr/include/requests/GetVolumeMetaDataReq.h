/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_GETVOLUMEMETADATAREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_GETVOLUMEMETADATAREQ_H_

#include <string>

#include "AmRequest.h"

namespace fds
{

struct GetVolumeMetaDataReq : AmRequest {
    GetVolumeMetaDataReq(fds_volid_t volId, const std::string & volumeName, CallbackPtr cb) :
            AmRequest(FDS_STAT_VOLUME, volId, volumeName, "", cb) {
        e2e_req_perf_ctx.type = AM_GET_VOLUME_META_OBJ_REQ;
        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }

    /// Status info to be returned
    fpi::VolumeStatus volumeStatus;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_GETVOLUMEMETADATAREQ_H_
