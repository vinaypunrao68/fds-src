/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_DETACHVOLUMEREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_DETACHVOLUMEREQ_H_

#include <string>

#include "AmRequest.h"

namespace fds
{

/**
 * AM request to locally detach a volume.
 * This request is not specific to a blob,
 * but needs to be in the blob wait queue
 * to call the callback and notify that the
 * detach is complete.
 */
struct DetachVolumeReq : public AmRequest {
    fds_int64_t token {invalid_vol_token};

    /**
     * Request constructor. Some of the fields
     * are not actually needed...the base blob
     * request class just expects them.
     */
    DetachVolumeReq(fds_volid_t        _volid,
                     const std::string& _vol_name,
                     CallbackPtr        cb) :
        AmRequest(FDS_DETACH_VOL, _volid, _vol_name, "", cb)
    {
        e2e_req_perf_ctx.type = PerfEventType::AM_VOLUME_DETACH_REQ,
        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_DETACHVOLUMEREQ_H_
