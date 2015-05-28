/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_ATTACHVOLUMEREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_ATTACHVOLUMEREQ_H_

#include <string>

#include "AmRequest.h"

namespace fds
{

/**
 * AM request to locally attach a volume.
 * This request is not specific to a blob,
 * but needs to be in the blob wait queue
 * to call the callback and notify that the
 * attach is complete.
 */
struct AttachVolumeReq : public AmRequest {
    fpi::VolumeAccessMode mode;
    fds_int64_t token {invalid_vol_token};

    /**
     * Request constructor. Some of the fields
     * are not actually needed...the base blob
     * request class just expects them.
     */
    AttachVolumeReq(fds_volid_t        _volid,
                     const std::string& _vol_name,
                     fpi::VolumeAccessMode const& _mode,
                     CallbackPtr        cb) :
        AmRequest(FDS_ATTACH_VOL, _volid, _vol_name, "", cb),
        mode(_mode)
    {
        e2e_req_perf_ctx.type = PerfEventType::AM_VOLUME_ATTACH_REQ,
        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }
    ~AttachVolumeReq() override = default;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_ATTACHVOLUMEREQ_H_
