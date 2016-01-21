/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_UPDATECATALOGREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_UPDATECATALOGREQ_H_

#include <string>
#include <vector>

#include "CommitBlobTxReq.h"
#include "PutBlobReq.h"

namespace fds
{

struct UpdateCatalogReq: public AmRequest,
                         public AmTxReq
{
    using buffer_type = boost::shared_ptr<std::string>;

    // ID for object
    std::unordered_map<ObjectID, std::pair<uint64_t, size_t>, ObjectHash> object_list;

    // Metadata changes
    template <typename T>
    using shared = boost::shared_ptr<T>;
    using meta_map_type = std::map<std::string, std::string>;
    shared<meta_map_type> metadata;

    fds_int32_t blob_mode;

    // Parent PutBlob request
    AmRequest* parent;

    bool atomic;

    /* Final metadata view after catalog update */
    std::map<std::string, std::string> final_meta_data;

    /* Final blob size after catalog update */
    size_t final_blob_size;

    explicit inline UpdateCatalogReq(PutBlobReq* blobReq, bool const _atomic = true);
    inline UpdateCatalogReq(CommitBlobTxReq* blobReq);

    ~UpdateCatalogReq() override = default;
};

UpdateCatalogReq::UpdateCatalogReq(PutBlobReq* blobReq, bool const _atomic)
    : AmRequest(FDS_CAT_UPD,
                blobReq->io_vol_id,
                blobReq->volume_name,
                blobReq->getBlobName(),
                nullptr,
                0,
                0),
      AmTxReq(blobReq->tx_desc),
      metadata(blobReq->metadata),
      blob_mode(blobReq->blob_mode),
      parent(blobReq),
      atomic(_atomic)
{
    qos_perf_ctx.type = PerfEventType::AM_PUT_QOS;
    hash_perf_ctx.type = PerfEventType::AM_PUT_HASH;
    dm_perf_ctx.type = PerfEventType::AM_PUT_DM;
    sm_perf_ctx.type = PerfEventType::AM_PUT_SM;

    e2e_req_perf_ctx.type = PerfEventType::AM_PUT_DM;
    fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
}

UpdateCatalogReq::UpdateCatalogReq(CommitBlobTxReq* blobReq)
    : AmRequest(FDS_CAT_UPD,
                blobReq->io_vol_id,
                blobReq->volume_name,
                blobReq->getBlobName(),
                nullptr,
                0,
                0),
      AmTxReq(blobReq->tx_desc),
      metadata(new meta_map_type()),
      parent(blobReq),
      atomic(true)
{
    qos_perf_ctx.type = PerfEventType::AM_PUT_QOS;
    hash_perf_ctx.type = PerfEventType::AM_PUT_HASH;
    dm_perf_ctx.type = PerfEventType::AM_PUT_DM;
    sm_perf_ctx.type = PerfEventType::AM_PUT_SM;

    e2e_req_perf_ctx.type = PerfEventType::AM_PUT_DM;
    fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
}


}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_UPDATECATALOGREQ_H_
