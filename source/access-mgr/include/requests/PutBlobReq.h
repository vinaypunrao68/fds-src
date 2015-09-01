/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_PUTBLOBREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_PUTBLOBREQ_H_

#include <string>

#include "AmRequest.h"

namespace fds
{

struct PutBlobReq
    :   public AmMultiReq,
        public AmTxReq
{
    // Calculated object id
    ObjectID obj_id;

    /// Shared pointer to data.
    boost::shared_ptr<std::string> dataPtr;

    /// Used for putBlobOnce scenarios.
    boost::shared_ptr< std::map<std::string, std::string> > metadata;
    fds_int32_t blob_mode;

    /* Final metadata view after catalog update */
    fpi::FDSP_MetaDataList final_meta_data;

    /* Final blob size after catalog update */
    fds_uint64_t final_blob_size;

    /// Constructor used on regular putBlob requests.
    inline PutBlobReq(fds_volid_t _volid,
               const std::string& _volumeName,
               const std::string& _blob_name,
               fds_uint64_t _blob_offset,
               fds_uint64_t _data_len,
               boost::shared_ptr<std::string> _data,
               BlobTxId::ptr _txDesc,
               CallbackPtr _cb);

    /// Constructor used on putBlobOnce requests.
    inline PutBlobReq(fds_volid_t          _volid,
               const std::string&   _volumeName,
               const std::string&   _blob_name,
               fds_uint64_t         _blob_offset,
               fds_uint64_t         _data_len,
               boost::shared_ptr<std::string> _data,
               fds_int32_t          _blobMode,
               boost::shared_ptr< std::map<std::string, std::string> >& _metadata,
               CallbackPtr _cb);

    void setTxId(fds_uint64_t const txId) {
        // We only expect to need to set this in putBlobOnce cases
        fds_verify(tx_desc == NULL);
        tx_desc = BlobTxId::ptr(new BlobTxId(txId));
    }

    ~PutBlobReq() override = default;
};

PutBlobReq::PutBlobReq(fds_volid_t _volid,
                       const std::string& _volName,
                       const std::string& _blob_name,
                       fds_uint64_t _blob_offset,
                       fds_uint64_t _data_len,
                       boost::shared_ptr<std::string> _data,
                       BlobTxId::ptr _txDesc,
                       CallbackPtr _cb)
    : AmMultiReq(FDS_PUT_BLOB, _volid, _volName, _blob_name, _cb, _blob_offset, _data_len),
    AmTxReq(_txDesc),
    final_meta_data(),
    final_blob_size(0ULL),
    dataPtr(_data)
{
    qos_perf_ctx.type = PerfEventType::AM_PUT_QOS;
    hash_perf_ctx.type = PerfEventType::AM_PUT_HASH;
    dm_perf_ctx.type = PerfEventType::AM_PUT_DM;
    sm_perf_ctx.type = PerfEventType::AM_PUT_SM;
    setResponseCount(2);

    e2e_req_perf_ctx.type = PerfEventType::AM_PUT_OBJ_REQ;
    fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
}

PutBlobReq::PutBlobReq(fds_volid_t          _volid,
                       const std::string&   _volName,
                       const std::string&   _blob_name,
                       fds_uint64_t         _blob_offset,
                       fds_uint64_t         _data_len,
                       boost::shared_ptr<std::string> _data,
                       fds_int32_t          _blobMode,
                       boost::shared_ptr< std::map<std::string, std::string> >& _metadata,
                       CallbackPtr _cb)
    : AmMultiReq(FDS_PUT_BLOB_ONCE, _volid, _volName, _blob_name, _cb, _blob_offset, _data_len),
    blob_mode(_blobMode),
    metadata(_metadata),
    final_meta_data(),
    final_blob_size(0ULL),
    dataPtr(_data)
{
    qos_perf_ctx.type = PerfEventType::AM_PUT_QOS;
    hash_perf_ctx.type = PerfEventType::AM_PUT_HASH;
    dm_perf_ctx.type = PerfEventType::AM_PUT_DM;
    sm_perf_ctx.type = PerfEventType::AM_PUT_SM;
    setResponseCount(2);

    e2e_req_perf_ctx.type = PerfEventType::AM_PUT_OBJ_REQ;
    fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
}

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_PUTBLOBREQ_H_
