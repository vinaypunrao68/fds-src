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
    size_t count;
    fds_uint64_t offset;
    std::string pattern;
    fpi::BlobListOrder orderBy;
    fpi::PatternSemantics patternSemantics;
    bool descending;
    std::string delimiter;

    VolumeContentsReq(fds_volid_t _volid,
                      std::string& bucketName,
                      size_t const _count,
                      int64_t _offset,
                      std::string const& _pattern,
                      fpi::PatternSemantics _patternSem,
                      fpi::BlobListOrder _orderBy,
                      bool _descending,
                      std::string const& _delimiter,
                      CallbackPtr cb)
            :   AmRequest(FDS_VOLUME_CONTENTS, _volid, bucketName, "", cb),
                count(_count), offset(_offset), pattern(_pattern), orderBy(_orderBy),
                patternSemantics(_patternSem), descending(_descending),
                delimiter(_delimiter)
    {
        e2e_req_perf_ctx.type = PerfEventType::AM_VOLUME_CONTENTS_REQ;
        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_VOLUMECONTENTSREQ_H_
