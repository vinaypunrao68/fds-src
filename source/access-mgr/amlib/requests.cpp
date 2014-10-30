/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#include <map>
#include <string>

#include "StorHvisorNet.h"
#include "requests/requests.h"

extern StorHvCtrl *storHvisor;

namespace fds
{

const fds_uint64_t VolumeContentsReq::fds_sh_volume_list_magic;
const fds_uint64_t VolumeStatsReq::fds_sh_volume_stats_magic;

GetBlobReq::GetBlobReq(fds_volid_t _volid,
                       const std::string& _volumeName,
                       const std::string& _blob_name,  // same as objKey
                       fds_uint64_t _blob_offset,
                       fds_uint64_t _data_len,
                       char* _data_buf,
                       CallbackPtr cb)
    : AmRequest(FDS_GET_BLOB, _volid, _volumeName, _blob_name, cb, _blob_offset,
                 _data_len, _data_buf) {
    stopwatch.start();

    std::string vol_str = std::string("volume: ") + std::to_string(io_vol_id);
    qos_perf_ctx.type = AM_GET_QOS;
    qos_perf_ctx.name = vol_str;
    qos_perf_ctx.reset_volid(io_vol_id);
    hash_perf_ctx.type = AM_GET_HASH;
    hash_perf_ctx.name = vol_str;
    hash_perf_ctx.reset_volid(io_vol_id);
    dm_perf_ctx.type = AM_GET_DM;
    dm_perf_ctx.name = vol_str;
    dm_perf_ctx.reset_volid(io_vol_id);
    sm_perf_ctx.type = AM_GET_SM;
    sm_perf_ctx.name = vol_str;
    sm_perf_ctx.reset_volid(io_vol_id);

    e2e_req_perf_ctx.type = AM_GET_OBJ_REQ;
    fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
}

GetBlobReq::~GetBlobReq()
{
    storHvisor->getCounters().gets_latency.update(stopwatch.getElapsedNanos());
}

PutBlobReq::PutBlobReq(fds_volid_t _volid,
                       const std::string& _volName,
                       const std::string& _blob_name,  // same as objKey
                       fds_uint64_t _blob_offset,
                       fds_uint64_t _data_len,
                       char* _data_buf,
                       BlobTxId::ptr _txDesc,
                       fds_bool_t _last_buf,
                       BucketContext* _bucket_ctxt,
                       PutPropertiesPtr _put_props,
                       void* _req_context,
                       CallbackPtr _cb) :
    AmRequest(FDS_PUT_BLOB, _volid, _volName, _blob_name, _cb, _blob_offset, _data_len, _data_buf),
    lastBuf(_last_buf),
    bucket_ctxt(_bucket_ctxt),
    ObjKey(_blob_name),
    putProperties(_put_props),
    req_context(_req_context),
    tx_desc(_txDesc),
    respAcks(2),
    retStatus(ERR_OK)
{
    stopwatch.start();

    std::string vol_str = std::string("volume: ") + std::to_string(io_vol_id);
    qos_perf_ctx.type = AM_PUT_QOS;
    qos_perf_ctx.name = vol_str;
    qos_perf_ctx.reset_volid(io_vol_id);
    hash_perf_ctx.type = AM_PUT_HASH;
    hash_perf_ctx.name = vol_str;
    hash_perf_ctx.reset_volid(io_vol_id);
    dm_perf_ctx.type = AM_PUT_DM;
    dm_perf_ctx.name = vol_str;
    dm_perf_ctx.reset_volid(io_vol_id);
    sm_perf_ctx.type = AM_PUT_SM;
    sm_perf_ctx.name = vol_str;
    sm_perf_ctx.reset_volid(io_vol_id);

    e2e_req_perf_ctx.type = AM_PUT_OBJ_REQ;
    fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
}

PutBlobReq::PutBlobReq(fds_volid_t          _volid,
                       const std::string&   _volName,
                       const std::string&   _blob_name,  // same as objKey
                       fds_uint64_t         _blob_offset,
                       fds_uint64_t         _data_len,
                       char*                _data_buf,
                       fds_int32_t          _blobMode,
                       boost::shared_ptr< std::map<std::string, std::string> >& _metadata,
                       CallbackPtr _cb) :
    AmRequest(FDS_PUT_BLOB_ONCE, _volid, _volName, _blob_name, _cb,
              _blob_offset, _data_len, _data_buf),
    ObjKey(_blob_name),
    tx_desc(nullptr),
    blobMode(_blobMode),
    metadata(_metadata),
    respAcks(2),
    retStatus(ERR_OK)
{
    stopwatch.start();

    std::string vol_str = std::string("volume: ") + std::to_string(io_vol_id);
    qos_perf_ctx.type = AM_PUT_QOS;
    qos_perf_ctx.name = vol_str;
    qos_perf_ctx.reset_volid(io_vol_id);
    hash_perf_ctx.type = AM_PUT_HASH;
    hash_perf_ctx.name = vol_str;
    hash_perf_ctx.reset_volid(io_vol_id);
    dm_perf_ctx.type = AM_PUT_DM;
    dm_perf_ctx.name = vol_str;
    dm_perf_ctx.reset_volid(io_vol_id);
    sm_perf_ctx.type = AM_PUT_SM;
    sm_perf_ctx.name = vol_str;
    sm_perf_ctx.reset_volid(io_vol_id);

    e2e_req_perf_ctx.type = AM_PUT_OBJ_REQ;
    fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
}

PutBlobReq::~PutBlobReq()
{
    storHvisor->getCounters().puts_latency.update(stopwatch.getElapsedNanos());
}

void
PutBlobReq::notifyResponse(const Error &e) {
    fds_verify(respAcks > 0);
    if (0 == --respAcks) {
        // Call back to processing layer
        proc_cb(e);
    }
}

void PutBlobReq::notifyResponse(StorHvQosCtrl *qos_ctrl, const Error &e)
{
    int cnt;
    /* NOTE: There is a race here in setting the error from
     * update catalog from dm and put object from sm in an error case
     * Most of the case the first error will win.  This should be ok for
     * now, in the event its' not we need a seperate lock here
     */
    if (retStatus == ERR_OK) {
        retStatus = e;
    }

    cnt = --respAcks;

    fds_assert(cnt >= 0);
    DBG(LOGDEBUG << "cnt: " << cnt << e);

    if (cnt == 0) {
        if (storHvisor->toggleNewPath) {
            if ((io_type == FDS_PUT_BLOB_ONCE) && (e == ERR_OK)) {
                // Push the commited update to the cache and remove from manager
                // We push here because we ONCE messages don't have an explicit
                // commit and here is where we know we've actually committed
                // to SM and DM.
                // TODO(Andrew): Inserting the entire tx transaction currently
                // assumes that the tx descriptor has all of the contents needed
                // for a blob descriptor (e.g., size, version, etc..). Today this
                // is true for S3/Swift and doesn't get used anyways for block (so
                // the actual cached descriptor for block will not be correct).
                AmTxDescriptor::ptr txDescriptor;
                fds_verify(storHvisor->amTxMgr->getTxDescriptor(*tx_desc,
                                                                txDescriptor) == ERR_OK);
                fds_verify(storHvisor->amCache->putTxDescriptor(txDescriptor) == ERR_OK);
                fds_verify(storHvisor->amTxMgr->removeTx(*tx_desc) == ERR_OK);
            }
            cb->call(e);
            qos_ctrl->markIODone(this);
        } else {
            // Call back to processing layer
            proc_cb(e);
        }
    }
}

}  // namespace fds
