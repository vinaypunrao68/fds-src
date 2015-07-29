/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_RENAMEBLOBREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_RENAMEBLOBREQ_H_

#include <string>

#include "AmRequest.h"

namespace fds
{

class RenameBlobReq : public AmRequest {
  public:
    /**
     * Request constructor. Some of the fields
     * are not actually needed...the base blob
     * request class just expects them.
     */
    RenameBlobReq(fds_volid_t          _volid,
                  const std::string   &_vol_name,
                  const std::string   &_blob_name,
                  const std::string   &_new_blob_name,
                  CallbackPtr         cb) :
            AmRequest(FDS_RENAME_BLOB, _volid, _vol_name, _blob_name, cb),
            new_blob_name(_new_blob_name)
    {
        e2e_req_perf_ctx.type = PerfEventType::AM_RENAME_BLOB_REQ;
        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }

    std::string const new_blob_name;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_RENAMEBLOBREQ_H_
