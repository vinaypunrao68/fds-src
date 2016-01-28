/*
 * Copyright 2013-2016 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_GETOBJECTREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_GETOBJECTREQ_H_

#include <string>
#include <vector>

#include "AmRequest.h"

namespace fds
{

struct GetBlobReq;

struct GetObjectReq: public AmRequest {
    using buffer_type = boost::shared_ptr<std::string>;

    // ID for object
    ObjectID::ptr obj_id;

    // Data object
    buffer_type& obj_data;

    // Parent GetBlob request
    GetBlobReq* blobReq;

    inline GetObjectReq(GetBlobReq* blobReq, buffer_type& buf, ObjectID::ptr _obj_id);

    ~GetObjectReq() override = default;
};

GetObjectReq::GetObjectReq(GetBlobReq* blobReq, buffer_type& buf, ObjectID::ptr _obj_id)
    : AmRequest(FDS_SM_GET_OBJECT, blobReq->io_vol_id, "", "", nullptr, 0, 0),
      obj_id(_obj_id),
      obj_data(buf),
      blobReq(blobReq)
{
    qos_perf_ctx.type = PerfEventType::AM_GET_QOS;
    hash_perf_ctx.type = PerfEventType::AM_GET_HASH;
    dm_perf_ctx.type = PerfEventType::AM_GET_DM;
    sm_perf_ctx.type = PerfEventType::AM_GET_SM;

    e2e_req_perf_ctx.type = PerfEventType::AM_GET_SM;
    fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
}


}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_GETOBJECTREQ_H_
