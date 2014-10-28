/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_PUTBLOBREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_PUTBLOBREQ_H_

#include <string>

#include "AmRequest.h"

namespace fds
{

struct StorHvQosCtrl;

struct PutBlobReq: public AmRequest {
  public:
    // TODO(Andrew): Fields that could use some cleanup.
    // We can mostly remove these with the new callback mechanism
    BucketContext *bucket_ctxt;
    std::string ObjKey;
    PutPropertiesPtr putProperties;
    void *req_context;
    void *callback_data;
    fds_bool_t lastBuf;

    // Needed fields
    BlobTxId::ptr tx_desc;
    fds_uint64_t dmtVersion;

    /// Used for putBlobOnce scenarios.
    boost::shared_ptr< std::map<std::string, std::string> > metadata;
    fds_int32_t blobMode;

    /* ack cnt for responses, decremented when response from SM and DM come back */
    std::atomic<int> respAcks;
    /* Return status for completion callback */
    Error retStatus;

    /// Constructor used on regular putBlob requests.
    PutBlobReq(fds_volid_t _volid,
               const std::string& _volumeName,
               const std::string& _blob_name,
               fds_uint64_t _blob_offset,
               fds_uint64_t _data_len,
               char* _data_buf,
               BlobTxId::ptr _txDesc,
               fds_bool_t _last_buf,
               BucketContext* _bucket_ctxt,
               PutPropertiesPtr _put_props,
               void* _req_context,
               CallbackPtr _cb);

    /// Constructor used on putBlobOnce requests.
    PutBlobReq(fds_volid_t          _volid,
               const std::string&   _volumeName,
               const std::string&   _blob_name,
               fds_uint64_t         _blob_offset,
               fds_uint64_t         _data_len,
               char*                _data_buf,
               fds_int32_t          _blobMode,
               boost::shared_ptr< std::map<std::string, std::string> >& _metadata,
               CallbackPtr _cb);

    fds_bool_t isLastBuf() const {
        return lastBuf;
    }

    std::string getEtag() const {
        std::string etag = "";
        if (putProperties != NULL) {
            etag = putProperties->md5;
        }
        return etag;
    }

    void setTxId(const BlobTxId &txId) {
        // We only expect to need to set this in putBlobOnce cases
        fds_verify(tx_desc == NULL);
        tx_desc = BlobTxId::ptr(new BlobTxId(txId));
    }

    virtual ~PutBlobReq();

    void notifyResponse(const Error &e);
    void notifyResponse(StorHvQosCtrl *qos_ctrl, const Error &e);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_PUTBLOBREQ_H_
