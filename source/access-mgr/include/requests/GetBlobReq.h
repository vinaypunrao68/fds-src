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

struct GetBlobReq: public AmMultiReq {
    fds_bool_t get_metadata;

    fds_bool_t metadata_cached;

    BlobDescriptor::ptr blobDesc;

    // IDs used to provide a consistent read across objects
    std::vector<ObjectID::ptr> object_ids;

    inline GetBlobReq(fds_volid_t _volid,
                      const std::string& _volumeName,
                      const std::string& _blob_name,
                      CallbackPtr cb,
                      fds_uint64_t _blob_offset,
                      fds_uint64_t _data_len);

    ~GetBlobReq() override = default;
};

GetBlobReq::GetBlobReq(fds_volid_t _volid,
                       const std::string& _volumeName,
                       const std::string& _blob_name,  // same as objKey
                       CallbackPtr cb,
                       fds_uint64_t _blob_offset,
                       fds_uint64_t _data_len)
    : AmMultiReq(FDS_GET_BLOB, _volid, _volumeName, _blob_name, cb, _blob_offset, _data_len),
      get_metadata(false), metadata_cached(false)
{
    qos_perf_ctx.type = PerfEventType::AM_GET_QOS;
    hash_perf_ctx.type = PerfEventType::AM_GET_HASH;
    dm_perf_ctx.type = PerfEventType::AM_GET_DM;
    sm_perf_ctx.type = PerfEventType::AM_GET_SM;

    e2e_req_perf_ctx.type = PerfEventType::AM_GET_BLOB_REQ;
    fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
}


}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_GETBLOBREQ_H_
