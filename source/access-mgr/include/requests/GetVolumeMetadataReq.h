/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_GETVOLUMEMETADATAREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_GETVOLUMEMETADATAREQ_H_

#include <string>

#include "AmRequest.h"

namespace fds
{

struct GetVolumeMetadataReq : public AmRequest {
    GetVolumeMetadataReq(fds_volid_t _volid,
                         const std::string   &_vol_name,
                         CallbackPtr cb) :
            AmRequest(FDS_GET_VOLUME_METADATA, _volid, _vol_name, "", cb) {
        e2e_req_perf_ctx.type = PerfEventType::AM_GET_VOLUME_METADATA_REQ;
        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_GETVOLUMEMETADATAREQ_H_
