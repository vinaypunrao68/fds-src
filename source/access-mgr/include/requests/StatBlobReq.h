/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_STATBLOBREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_STATBLOBREQ_H_

#include <string>

#include "AmRequest.h"

namespace fds
{

class StatBlobReq : public AmRequest {
  public:
    fds_volid_t base_vol_id;

    /**
     * Request constructor. Some of the fields
     * are not actually needed...the base blob
     * request class just expects them.
     */
    StatBlobReq(fds_volid_t          _volid,
                const std::string   &_vol_name,
                const std::string   &_blob_name,
                fds_uint64_t         _blob_offset,
                fds_uint64_t         _data_len,
                char                *_data_buf,
                CallbackPtr cb) :
            AmRequest(FDS_STAT_BLOB, _volid, _blob_name, _blob_offset,
                       _data_len, _data_buf, cb) {
        volume_name = _vol_name;
        e2e_req_perf_ctx.type = AM_STAT_BLOB_OBJ_REQ;
        e2e_req_perf_ctx.name = "volume:" + std::to_string(_volid);
        e2e_req_perf_ctx.reset_volid(_volid);

        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }

    StatBlobReq(fds_volid_t          _volid,
                const std::string   &_vol_name,
                const std::string   &_blob_name,
                CallbackPtr cb) :
            AmRequest(FDS_STAT_BLOB, _volid, _blob_name, 0,
                       0, NULL, cb) {
        volume_name = _vol_name;
        e2e_req_perf_ctx.type = AM_STAT_BLOB_OBJ_REQ;
        e2e_req_perf_ctx.name = "volume:" + std::to_string(vol_id);
        e2e_req_perf_ctx.reset_volid(vol_id);

        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }

    virtual ~StatBlobReq() {
        fds::PerfTracer::tracePointEnd(e2e_req_perf_ctx);
    }
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_STATBLOBREQ_H_
