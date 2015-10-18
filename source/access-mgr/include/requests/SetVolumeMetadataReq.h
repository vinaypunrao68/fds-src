/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_SETVOLUMEMETADATAREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_SETVOLUMEMETADATAREQ_H_

#include <string>

#include "AmRequest.h"

namespace fds
{

struct SetVolumeMetadataReq :
    public AmRequest
{
    typedef boost::shared_ptr<std::map<std::string, std::string>> shared_meta_type;
    shared_meta_type metadata;

    SetVolumeMetadataReq(fds_volid_t _volid,
                         const std::string   &_vol_name,
                         shared_meta_type _metaDataList,
                         CallbackPtr cb) :
            AmRequest(FDS_SET_VOLUME_METADATA, _volid, _vol_name, "", cb),
            metadata(_metaDataList) {
        e2e_req_perf_ctx.type = PerfEventType::AM_SET_VOLUME_METADATA_REQ;
        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }

    fds_uint64_t vol_sequence {0};
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_SETVOLUMEMETADATAREQ_H_
