/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_GETOBJECTREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_GETOBJECTREQ_H_

#include <string>
#include <vector>

#include "AmRequest.h"

namespace fds
{

struct GetObjectReq: public AmRequest {
    // ID for object
    ObjectID::ptr obj_id;

    // Data object
    boost::shared_ptr<std::string> obj_data;

    inline GetObjectReq(fds_volid_t _volid, ObjectID::ptr _obj_id);

    ~GetObjectReq() override = default;
};

GetObjectReq::GetObjectReq(fds_volid_t _volid, ObjectID::ptr _obj_id)
    : AmRequest(FDS_SM_GET_OBJECT, _volid, "", "", nullptr, 0, 0),
      obj_id(_obj_id)
{
    qos_perf_ctx.type = PerfEventType::AM_GET_QOS;
    hash_perf_ctx.type = PerfEventType::AM_GET_HASH;
    dm_perf_ctx.type = PerfEventType::AM_GET_DM;
    sm_perf_ctx.type = PerfEventType::AM_GET_SM;

    e2e_req_perf_ctx.type = PerfEventType::AM_GET_OBJ_REQ;
    fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
}


}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_GETOBJECTREQ_H_
