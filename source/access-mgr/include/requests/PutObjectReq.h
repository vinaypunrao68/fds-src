/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_PUTOBJECTREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_PUTOBJECTREQ_H_

#include <string>
#include <vector>

#include "PutBlobReq.h"

namespace fds
{

struct PutObjectReq: public AmRequest {
    using buffer_type = boost::shared_ptr<std::string>;

    // ID for object
    ObjectID obj_id;

    /// Shared pointer to data.
    buffer_type dataPtr;

    // Parent PutBlob request
    PutBlobReq* parent;

    explicit inline PutObjectReq(PutBlobReq* blobReq);
    inline PutObjectReq(fds_volid_t const vol_id,
                        std::string const vol_name,
                        std::string const blob_name,
                        buffer_type buffer,
                        ObjectID const& id,
                        size_t const length);

    ~PutObjectReq() override = default;
};

PutObjectReq::PutObjectReq(PutBlobReq* blobReq)
    : AmRequest(FDS_SM_PUT_OBJECT,
                blobReq->io_vol_id,
                blobReq->volume_name,
                blobReq->getBlobName(),
                nullptr,
                blobReq->blob_offset,
                blobReq->data_len),
      obj_id(blobReq->obj_id),
      dataPtr(blobReq->dataPtr),
      parent(blobReq)
{
    qos_perf_ctx.type = PerfEventType::AM_PUT_QOS;
    hash_perf_ctx.type = PerfEventType::AM_PUT_HASH;
    dm_perf_ctx.type = PerfEventType::AM_PUT_DM;
    sm_perf_ctx.type = PerfEventType::AM_PUT_SM;

    e2e_req_perf_ctx.type = PerfEventType::AM_PUT_SM;
    fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
}

PutObjectReq::PutObjectReq(fds_volid_t const vol_id,
             std::string const vol_name,
             std::string const blob_name,
             buffer_type buffer,
             ObjectID const& id,
             size_t const length)
    : AmRequest(FDS_SM_PUT_OBJECT, vol_id, vol_name, blob_name, nullptr, 0, length),
      obj_id(id),
      dataPtr(buffer),
      parent(nullptr)
{
    qos_perf_ctx.type = PerfEventType::AM_PUT_QOS;
    hash_perf_ctx.type = PerfEventType::AM_PUT_HASH;
    dm_perf_ctx.type = PerfEventType::AM_PUT_DM;
    sm_perf_ctx.type = PerfEventType::AM_PUT_SM;

    e2e_req_perf_ctx.type = PerfEventType::AM_PUT_SM;
    fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
}

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_PUTOBJECTREQ_H_
