/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_UPDATECATALOGREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_UPDATECATALOGREQ_H_

#include <string>
#include <vector>

#include "AmRequest.h"
#include "PutBlobReq.h"

namespace fds
{

struct UpdateCatalogReq: public AmRequest,
                         public AmTxReq
{
    using buffer_type = boost::shared_ptr<std::string>;

    // ID for object
    ObjectID obj_id;

    // Metadata changes
    boost::shared_ptr< std::map<std::string, std::string> > metadata;

    fds_int32_t blob_mode;

    // Parent PutBlob request
    PutBlobReq* parent;

    bool atomic;

    /* Final metadata view after catalog update */
    std::map<std::string, std::string> final_meta_data;

    /* Final blob size after catalog update */
    size_t final_blob_size;

    explicit inline UpdateCatalogReq(PutBlobReq* blobReq, bool const _atomic = true);

    ~UpdateCatalogReq() override = default;
};

UpdateCatalogReq::UpdateCatalogReq(PutBlobReq* blobReq, bool const _atomic)
    : AmRequest(FDS_CAT_UPD,
                blobReq->io_vol_id,
                blobReq->volume_name,
                blobReq->getBlobName(),
                nullptr,
                blobReq->blob_offset,
                blobReq->data_len),
      AmTxReq(blobReq->tx_desc),
      obj_id(blobReq->obj_id),
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


}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_UPDATECATALOGREQ_H_
