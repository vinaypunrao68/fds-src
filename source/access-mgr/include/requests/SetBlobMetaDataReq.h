/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_SETBLOBMETADATAREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_SETBLOBMETADATAREQ_H_

#include <string>

#include "fdsp/FDSP_types.h"
#include "AmRequest.h"

namespace fds
{

struct SetBlobMetaDataReq : AmRequest {
    BlobTxId::ptr tx_desc;
    boost::shared_ptr<FDS_ProtocolInterface::FDSP_MetaDataList> metaDataList;

    fds_uint64_t dmt_version;

    SetBlobMetaDataReq(fds_volid_t _volid,
                       const std::string   &_vol_name,
                       const std::string   &_blob_name,
                       BlobTxId::ptr _txDesc,
                       boost::shared_ptr<FDS_ProtocolInterface::FDSP_MetaDataList> _metaDataList,
                       CallbackPtr cb) :
            AmRequest(FDS_SET_BLOB_METADATA, _volid, _vol_name, _blob_name, cb),
            tx_desc(_txDesc),  metaDataList(_metaDataList) {
        e2e_req_perf_ctx.type = AM_SET_BLOB_META_OBJ_REQ;
        e2e_req_perf_ctx.name = "volume:" + std::to_string(vol_id);
        e2e_req_perf_ctx.reset_volid(vol_id);

        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }

    virtual ~SetBlobMetaDataReq() {
        fds::PerfTracer::tracePointEnd(e2e_req_perf_ctx);
    }

    boost::shared_ptr<FDS_ProtocolInterface::FDSP_MetaDataList> getMetaDataListPtr()
            const { return metaDataList; }
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_SETBLOBMETADATAREQ_H_
