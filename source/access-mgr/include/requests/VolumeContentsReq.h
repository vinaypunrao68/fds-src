/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_VOLUMECONTENTSREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_VOLUMECONTENTSREQ_H_

#include <string>

#include "AmRequest.h"

namespace fds
{

struct VolumeContentsReq: public AmRequest {
    size_t maxkeys;

    VolumeContentsReq(fds_volid_t _volid,
                      std::string& bucketName,
                      size_t const _max_keys,
                      CallbackPtr cb)
            :   AmRequest(FDS_VOLUME_CONTENTS, _volid, bucketName, "", cb),
                maxkeys(_max_keys)
    {
        e2e_req_perf_ctx.type = AM_VOLUME_CONTENTS_REQ;
        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_VOLUMECONTENTSREQ_H_
