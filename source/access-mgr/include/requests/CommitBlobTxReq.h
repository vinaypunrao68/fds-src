/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_REQUEST_COMMITBLOBTXREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_REQUEST_COMMITBLOBTXREQ_H_

#include <string>

#include "AmRequest.h"

namespace fds
{

struct CommitBlobTxReq : 
    public AmRequest,
    public AmTxReq
{
    /**
     * Request constructor. Some of the fields
     * are not actually needed...the base blob
     * request class just expects them.
     */
    CommitBlobTxReq(fds_volid_t        _volid,
                   const std::string &_vol_name,
                   const std::string &_blob_name,
                   BlobTxId::ptr _txDesc,
                   CallbackPtr        _cb) :
            AmRequest(FDS_COMMIT_BLOB_TX, _volid, _vol_name, _blob_name, _cb),
            AmTxReq(_txDesc),
            final_blob_size(0ULL),
            final_meta_data()
    {
        e2e_req_perf_ctx.type = PerfEventType::AM_COMMIT_BLOB_OBJ_REQ;
        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }

    fds_uint64_t final_blob_size;
    fpi::FDSP_MetaDataList final_meta_data;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUEST_COMMITBLOBTXREQ_H_
