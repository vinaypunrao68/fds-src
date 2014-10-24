/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_GETVOLUMEMETADATAREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_GETVOLUMEMETADATAREQ_H_

#include <string>

#include "FdsBlobReq.h"

namespace fds
{

struct GetVolumeMetaDataReq : FdsBlobReq {
    GetVolumeMetaDataReq(fds_volid_t volId, const std::string & volumeName, CallbackPtr cb) :
            FdsBlobReq(FDS_GET_VOLUME_METADATA, volId, "" , 0, 0, NULL, cb) {
        volume_name = volumeName;
        e2e_req_perf_ctx.type = AM_GET_VOLUME_META_OBJ_REQ;
        e2e_req_perf_ctx.name = "volume:" + std::to_string(volId);
        e2e_req_perf_ctx.reset_volid(volId);

        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }

    virtual ~GetVolumeMetaDataReq() {
        fds::PerfTracer::tracePointEnd(e2e_req_perf_ctx);
    }

    /// Metadata to be returned
    fpi::FDSP_VolumeMetaData volumeMetadata;

    typedef std::function<void (const Error&)> GetVolMetadataProcCb;
    GetVolMetadataProcCb processorCb;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_GETVOLUMEMETADATAREQ_H_
