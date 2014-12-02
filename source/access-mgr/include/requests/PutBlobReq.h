/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_PUTBLOBREQ_H_
#define SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_PUTBLOBREQ_H_

#include <string>

#include "AmRequest.h"

namespace fds
{

class StorHvQosCtrl;

struct PutBlobReq
    :   public AmRequest,
        public AmTxReq
{
    // TODO(Andrew): Fields that could use some cleanup.
    // We can mostly remove these with the new callback mechanism
    BucketContext *bucket_ctxt;
    void *req_context;
    void *callback_data;
    fds_bool_t last_buf;

    // Needed fields
    fds_uint64_t dmt_version;

    /// Shared pointer to data. If this is used, the inherited raw pointer is NULL
    boost::shared_ptr<std::string> dataPtr;

    /// Used for putBlobOnce scenarios.
    boost::shared_ptr< std::map<std::string, std::string> > metadata;
    fds_int32_t blob_mode;

    /* ack cnt for responses, decremented when response from SM and DM come back */
    std::atomic<int> resp_acks;
    /* Return status for completion callback */
    Error ret_status;

    /// Constructor used on regular putBlob requests.
    PutBlobReq(fds_volid_t _volid,
               const std::string& _volumeName,
               const std::string& _blob_name,
               fds_uint64_t _blob_offset,
               fds_uint64_t _data_len,
               boost::shared_ptr<std::string> _data,
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
               boost::shared_ptr<std::string> _data,
               fds_int32_t          _blobMode,
               boost::shared_ptr< std::map<std::string, std::string> >& _metadata,
               CallbackPtr _cb);

    std::string getEtag() const {
        return put_properties ? put_properties->md5 : std::string();
    }

    void setTxId(const BlobTxId &txId) {
        // We only expect to need to set this in putBlobOnce cases
        fds_verify(tx_desc == NULL);
        tx_desc = BlobTxId::ptr(new BlobTxId(txId));
    }

    virtual ~PutBlobReq();

    void notifyResponse(const Error &e);
    void notifyResponse(StorHvQosCtrl *qos_ctrl, const Error &e);

 private:
    PutPropertiesPtr put_properties;
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_REQUESTS_PUTBLOBREQ_H_
