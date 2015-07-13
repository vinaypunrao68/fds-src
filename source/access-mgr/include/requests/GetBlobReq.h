/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_GETBLOBREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_GETBLOBREQ_H_

#include <string>
#include <vector>

#include "AmRequest.h"

namespace fds
{

struct GetBlobReq: public AmRequest {
    fds_bool_t get_metadata;

    fds_bool_t metadata_cached;
    // TODO(bszmyd): Mon 23 Mar 2015 02:59:55 AM PDT
    // Take this out when we support transactions.
    fds_bool_t retry { false };

    // IDs used to provide a consistent read across objects
    std::vector<ObjectID::ptr> object_ids;

    inline GetBlobReq(fds_volid_t _volid,
                      const std::string& _volumeName,
                      const std::string& _blob_name,
                      CallbackPtr cb,
                      fds_uint64_t _blob_offset,
                      fds_uint64_t _data_len);

    ~GetBlobReq() override = default;

    void setResponseCount(uint16_t const cnt) {
        resp_acks = cnt;
    }

    void notifyResponse(const Error &e) {
        size_t acks_left = 0;
        {
            std::lock_guard<std::mutex> g(resp_lock);
            op_err = e.ok() ? op_err : e;
            acks_left = --resp_acks;
        }
        if (0 == acks_left) {
            // Call back to processing layer
            proc_cb(op_err);
        }
    }

 private:
    /* ack cnt for responses, decremented when response from SM and DM come back */
    std::mutex resp_lock;
    size_t resp_acks;
    Error op_err {ERR_OK};
};

GetBlobReq::GetBlobReq(fds_volid_t _volid,
                       const std::string& _volumeName,
                       const std::string& _blob_name,  // same as objKey
                       CallbackPtr cb,
                       fds_uint64_t _blob_offset,
                       fds_uint64_t _data_len)
    : AmRequest(FDS_GET_BLOB, _volid, _volumeName, _blob_name, cb, _blob_offset, _data_len),
      get_metadata(false), metadata_cached(false), resp_acks(0)
{
    qos_perf_ctx.type = PerfEventType::AM_GET_QOS;
    hash_perf_ctx.type = PerfEventType::AM_GET_HASH;
    dm_perf_ctx.type = PerfEventType::AM_GET_DM;
    sm_perf_ctx.type = PerfEventType::AM_GET_SM;

    e2e_req_perf_ctx.type = PerfEventType::AM_GET_OBJ_REQ;
    fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
}


}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_GETBLOBREQ_H_
