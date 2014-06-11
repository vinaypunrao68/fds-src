/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <fdsp/FDSP_types.h>
#include <FDSNStat.h>
#include <native_api.h>
#include <StorHvisorNet.h>
#include <string>
extern StorHvCtrl *storHvisor;

namespace fds {

static int __reg;

FDS_NativeAPI::FDS_NativeAPI(FDSN_ClientType type) : clientType(type) {
    // FIXME: each client type should have a separate stat.
    if (!__reg) {
        __reg = 1;
        fdsn_register_stat();
    }
}

FDS_NativeAPI::~FDS_NativeAPI() {
}

void FDS_NativeAPI::initVolInfo(FDSP_VolumeInfoTypePtr vol_info,
                                const std::string& bucket_name) {
    vol_info->vol_name = std::string(bucket_name);
    vol_info->tennantId = 0;
    vol_info->localDomainId = 0;
    vol_info->globDomainId = 0;

    // Volume capacity is in MB
    vol_info->capacity = (1024*10);  // for now presetting to 10GB
    vol_info->maxQuota = 0;
    vol_info->volType = FDSP_VOL_S3_TYPE;

    vol_info->defReplicaCnt = 0;
    vol_info->defWriteQuorum = 0;
    vol_info->defReadQuorum = 0;
    vol_info->defConsisProtocol = FDSP_CONS_PROTO_STRONG;

    vol_info->volPolicyId = 50;  // default S3 policy desc ID
    vol_info->archivePolicyId = 0;
    vol_info->placementPolicy = 0;
    vol_info->appWorkload = FDSP_APP_WKLD_TRANSACTION;
    vol_info->mediaPolicy = FDSP_MEDIA_POLICY_HDD;
}

void FDS_NativeAPI::initVolDesc(FDSP_VolumeDescTypePtr vol_desc,
                                const std::string& bucket_name) {
    vol_desc->vol_name = std::string(bucket_name);
    vol_desc->tennantId = 0;
    vol_desc->localDomainId = 0;
    vol_desc->globDomainId = 0;

    // Volume capacity is in MB
    vol_desc->capacity = (1024*10);  // for now presetting to 10GB
    vol_desc->maxQuota = 0;
    vol_desc->volType = FDSP_VOL_S3_TYPE;

    vol_desc->defReplicaCnt = 0;
    vol_desc->defWriteQuorum = 0;
    vol_desc->defReadQuorum = 0;
    vol_desc->defConsisProtocol = FDSP_CONS_PROTO_STRONG;

    vol_desc->volPolicyId = 50;  // default S3 policy desc ID
    vol_desc->archivePolicyId = 0;
    vol_desc->placementPolicy = 0;
    vol_desc->appWorkload = FDSP_APP_WKLD_TRANSACTION;
    vol_desc->mediaPolicy = FDSP_MEDIA_POLICY_HDD;
}

/* Create a bucket */
void FDS_NativeAPI::CreateBucket(BucketContext *bucket_ctx,
                                 CannedAcl  canned_acl,
                                 void *req_ctxt,
                                 fdsnResponseHandler responseHandler,
                                 void *callback_data)
{
    int om_err = 0;
    fds_volid_t ret_id;
    // check the bucket is already attached.
    ret_id = storHvisor->vol_table->getVolumeUUID(bucket_ctx->bucketName);
    if (ret_id != invalid_vol_id) {
        LOGERROR << " S3 Bucket Already exsists  BucketID: " << ret_id;
        (responseHandler)(FDSN_StatusErrorBucketAlreadyExists, NULL, callback_data); //NOLINT
        return;
    }

    FDSP_VolumeInfoTypePtr vol_info(new FDSP_VolumeInfoType());
    initVolInfo(vol_info, bucket_ctx->bucketName);
    vol_info->volPolicyId = 50;  // default S3 policy desc ID

    // send the  bucket create request to OM

    om_err = storHvisor->om_client->pushCreateBucketToOM(vol_info);
    if (om_err != 0) {
        (responseHandler)(FDSN_StatusInternalError, NULL, callback_data);
        LOGWARN << "FDS_NativeAPI::CreateBucket bucket " << bucket_ctx->bucketName
                << " -- could't send create bucket request to OM";
        return;
    }

    (responseHandler)(FDSN_StatusOK, NULL, callback_data);
}


/* Get the bucket contents  or objets belonging to this bucket */
void FDS_NativeAPI::GetBucket(BucketContext *bucket_ctxt,
                              std::string prefix,
                              std::string marker,
                              std::string delimiter,
                              fds_uint32_t maxkeys,
                              void *req_context,
                              fdsnListBucketHandler handler,
                              void *callback_data)
{
    Error err(ERR_OK);
    fds_volid_t volid = invalid_vol_id;
    FdsBlobReq *blob_req = NULL;
    LOGNORMAL << "FDS_NativeAPI::GetBucket for bucket " << bucket_ctxt->bucketName;

    /* check if bucket is attached to this AM */
    if (storHvisor->vol_table->volumeExists(bucket_ctxt->bucketName)) {
        volid =  storHvisor->vol_table->getVolumeUUID(bucket_ctxt->bucketName);
        fds_verify(volid != invalid_vol_id);
    }

    // if bucket is not attached to this AM, before sending attach request to OM
    // create List request and put it to wait queue, to make sure that attach arrives
    // after we put a request to the wait queue and not before!

    /* create request */
    blob_req = new ListBucketReq(volid,
                                 bucket_ctxt,
                                 prefix,
                                 marker,
                                 delimiter,
                                 maxkeys,
                                 req_context,
                                 handler,
                                 callback_data);

    if (!blob_req) {
        (handler)(0, "", 0, NULL, 0, NULL, callback_data, FDSN_StatusInternalError);
        LOGERROR << "FDS_NativeAPI::GetBucket for bucket "
                 << bucket_ctxt->bucketName
                 << " -- failed to allocate ListBucketReq";
        return;
    }

    if (volid != invalid_vol_id) {
        /* bucket is already attached to this AM, enqueue IO */
        storHvisor->pushBlobReq(blob_req);
        return;
    } else {
        /* we are waiting for OM to tell us if bucket exists */
        storHvisor->vol_table->addBlobToWaitQueue(bucket_ctxt->bucketName, blob_req);
    }

    // if we are here, bucket is not attached to this AM, send test bucket msg to OM
    err = sendTestBucketToOM(bucket_ctxt->bucketName,
                             bucket_ctxt->accessKeyId,
                             bucket_ctxt->secretAccessKey);
    if (!err.ok()) {
        // we failed to send query to OM
        LOGWARN << "FDS_NativeAPI::GetBucket for bucket " << bucket_ctxt->bucketName
                << " -- could't find out from OM if bucket exists";
        // remove blob from wait queue (this will also call the callback)
        storHvisor->vol_table->completeWaitBlobsWithError(bucket_ctxt->bucketName, err);
    }
}

void FDS_NativeAPI::DeleteBucket(BucketContext* bucketCtxt,
                                 void *requestContext,
                                 fdsnResponseHandler handler,
                                 void *callbackData)
{
    fds_volid_t ret_id;
    LOGNORMAL << "FDS_NativeAPI:DeleteBucket for bucket " << bucketCtxt->bucketName;
    // check the bucket is already attached.
    ret_id = storHvisor->vol_table->getVolumeUUID(bucketCtxt->bucketName);
    if (ret_id == invalid_vol_id) {
        LOGERROR << " S3 Bucket  Does not exsists  BucketID: " << ret_id;
        (handler)(FDSN_StatusErrorBucketNotExists, NULL, callbackData);
        return;
    }

    FDSP_DeleteVolTypePtr volData(new FDSP_DeleteVolType());
    volData->vol_name  = std::string(bucketCtxt->bucketName);
    volData->domain_id = 0;
    // send the  bucket create request to OM

    storHvisor->om_client->pushDeleteBucketToOM(volData);
    /* TBD. Since this one is async call Error
       checking involved, need more discussiosn */
    (handler)(FDSN_StatusOK, NULL, callbackData);
    return;
}


/* This method sends modify volume (bucket) message directly to OM, independent
 * whether this AM has this bucket attached or not. This AM will modify qos params
 * in response to modify volume from OM  (in response to this method call) */
void FDS_NativeAPI::ModifyBucket(BucketContext *bucket_ctxt,
                                 const QosParams& qos_params,
                                 void *req_ctxt,
                                 fdsnResponseHandler handler,
                                 void *callback_data)
{
    int om_err = 0;
    double iops_min = qos_params.iops_min;
    double iops_max = qos_params.iops_max;
    FDSP_VolumeDescTypePtr voldesc(new FDSP_VolumeDescType());

    /* iops_min and iops_max in qos params must be values from 0 to 100,
     * and priority from 1 to 10 */
    if ((iops_min < 0) || (iops_min > 100) ||
        (iops_max < 0) || (iops_max > 100) ||
        (qos_params.relativePrio < 1) || (qos_params.relativePrio > 10)) {
        (handler)(FDSN_StatusInternalError, NULL, callback_data);
        LOGERROR << "FDS_NativeAPI::ModifyBucket bucket "
                 << bucket_ctxt->bucketName
                 << " sla (iops_min) and limit (iops_max) must be "
                 << "from 0 to 100, and priority from 1 to 10";
        return;
    }

    /* get absolute numbers for iops min and max */
    iops_min = iops_min * FDSN_QOS_PERF_NORMALIZER;
    iops_max = iops_max * FDSN_QOS_PERF_NORMALIZER;

    LOGNORMAL << "FDS_NativeAPI::ModifyBucket bucket " << bucket_ctxt->bucketName
              << " -- min_iops " << iops_min << " (sla " << qos_params.iops_min << ")"
              << ", max_iops " << iops_max << " (limit " << qos_params.iops_max << ")"
              << ", priority " << qos_params.relativePrio;


    /* send modify volume request to OM -- we don't care if bucket is attached to this AM,
     * this AM will modify bucket qos params in response to modify volume msg from OM */
    initVolDesc(voldesc, bucket_ctxt->bucketName);
    voldesc->volPolicyId = 0; /* 0 means don't modify actual policy id, just qos params */
    voldesc->iops_min = iops_min;
    voldesc->iops_max = iops_max;
    voldesc->rel_prio = qos_params.relativePrio;

    om_err = storHvisor->om_client->pushModifyBucketToOM(bucket_ctxt->bucketName,
                                                         voldesc);

    if (om_err != 0) {
        (handler)(FDSN_StatusInternalError, NULL, callback_data);
        LOGERROR << "FDS_NativeAPI::ModifyBucket bucket " << bucket_ctxt->bucketName
                 << " -- could't send modify bucket request to OM";
        return;
    }

    /* TODO: we reply with err_ok right away, even if error could happen on OM
     * to implement async call we need AM to handle msg responses from OM +
     * add 'admin' queue to volume queues with some rate and send msgs to OM when
     * we dequeue those requests (not here).
     */
    (handler)(FDSN_StatusOK, NULL, callback_data);
}

/* get bucket stats for all existing buckets from OM*/
void FDS_NativeAPI::GetBucketStats(void *req_ctxt,
                                   fdsnBucketStatsHandler resp_handler,
                                   void *callback_data)
{
    FdsBlobReq *blob_req = NULL;
    LOGNORMAL << "FDS_NativeAPI::GetBucketStats for all existing buckets";

    /* this request will go directly to OM,
       so not need to check if buckets are attached, etc. */

    blob_req = new BucketStatsReq(req_ctxt,
                                  resp_handler,
                                  callback_data);

    if (!blob_req) {
        (resp_handler)("", 0, NULL, req_ctxt, callback_data, FDSN_StatusOutOfMemory, NULL); //NOLINT
        LOGERROR << "FDS_NativeAPI::GetBucketStatus for all existing buckets "
                 << " -- failed to allocate BucketStatsReq";
        return;
    }

    /* this request will go to 'admin' queue which should always exist */
    storHvisor->pushBlobReq(blob_req);
}

void
FDS_NativeAPI::GetObject(BucketContextPtr bucket_ctxt,
                         std::string ObjKey,
                         GetConditions *get_cond,
                         fds_uint64_t startByte,
                         fds_uint64_t byteCount,
                         char* buffer,
                         fds_uint64_t buflen,
                         void *req_context,
                         fdsnGetObjectHandler getObjCallback,
                         void *callback_data) {
    Error err(ERR_OK);
    fds_volid_t volid = invalid_vol_id;
    fds_uint64_t start, end;
    FdsBlobReq *blob_req = NULL;
    LOGNORMAL << "FDS_NativeAPI::GetObject bucket " << bucket_ctxt->bucketName
              << " objKey " << ObjKey;

    /* check if bucket is attached to this AM */
    start = fds_rdtsc();
    if (storHvisor->vol_table->volumeExists(bucket_ctxt->bucketName)) {
        volid = storHvisor->vol_table->getVolumeUUID(bucket_ctxt->bucketName);
        fds_verify(volid != invalid_vol_id);
    }
    end = fds_rdtsc();
    fds_stat_record(STAT_FDSN, FDSN_GO_CHK_BKET_EXIST, start, end);

    // if bucket is not attached to this AM, before sending attach request to OM
    // create GetBlob request and put it to wait queue, to make sure that attach
    // arrives after we put a request to the wait queue and not before!

    /* create request */
    start = end;
    blob_req = new GetBlobReq(volid,
                              ObjKey,
                              startByte,
                              buflen,
                              buffer,
                              byteCount,
                              bucket_ctxt,
                              get_cond,
                              req_context,
                              getObjCallback,
                              callback_data);

    if (!blob_req) {
        (getObjCallback)(bucket_ctxt, req_context, 0, startByte, NULL, 0, "",
                         callback_data, FDSN_StatusOutOfMemory, NULL);
        LOGERROR << "FDS_NativeAPI::GetObject bucket "
                 << bucket_ctxt->bucketName
                 << " objKey " << ObjKey
                 << " -- failed to allocate GetBlobReq";
        return;
    }
    end = fds_rdtsc();
    fds_stat_record(STAT_FDSN, FDSN_GO_ALLOC_BLOB_REQ, start, end);

    if (volid != invalid_vol_id) {
        /* bucket is already attached to this AM, enqueue IO */
        start = end;
        storHvisor->pushBlobReq(blob_req);
        end = fds_rdtsc();
        fds_stat_record(STAT_FDSN, FDSN_GO_ENQUEUE_IO, start, end);
        return;
    } else {
        /* we are waiting for OM to tell us if bucket exists */
        start = end;
        storHvisor->vol_table->addBlobToWaitQueue(bucket_ctxt->bucketName, blob_req);
        fds_stat_record(STAT_FDSN, FDSN_GO_ADD_WAIT_QUEUE, start, end);
    }

    // if we are here, bucket is not attached to this AM, send test bucket msg to OM
    err = sendTestBucketToOM(bucket_ctxt->bucketName,
                             bucket_ctxt->accessKeyId,
                             bucket_ctxt->secretAccessKey);
    if (!err.ok()) {
        /* we failed to send query to OM */
        LOGERROR << "FDS_NativeAPI::GetObject bucket " << bucket_ctxt->bucketName
                 << " objKey " << ObjKey
                 << " -- could't find out from OM if bucket exists";
        // remove blob from wait queue (this will also call the callback)
        storHvisor->vol_table->completeWaitBlobsWithError(bucket_ctxt->bucketName, err);
    }
}

void FDS_NativeAPI::PutObject(BucketContext *bucket_ctxt,
                              std::string ObjKey,
                              PutPropertiesPtr put_properties,
                              void *req_context,
                              char *buffer,
                              fds_uint64_t startByte,
                              fds_uint64_t buflen,
                              fds_bool_t lastBuf,
                              fdsnPutObjectHandler putObjHandler,
                              void *callback_data) {
    Error err(ERR_OK);
    fds_volid_t volid = invalid_vol_id;
    FdsBlobReq *blob_req = NULL;
    LOGNORMAL << "bucket: " << bucket_ctxt->bucketName
              << " objKey: " << ObjKey
              << " startByte:" << startByte
              << " buffer-len: " << buflen
              << " last buffer: " << std::boolalpha << lastBuf;

    // check if bucket is attached to this AM
    if (storHvisor->vol_table->volumeExists(bucket_ctxt->bucketName)) {
        volid = storHvisor->vol_table->getVolumeUUID(bucket_ctxt->bucketName);
        fds_verify(volid != invalid_vol_id);
    }

    // if bucket is not attached to this AM, before sending attach request to OM
    // create Put request and put it to wait queue, to make sure that attach arrives
    // after we put a request to the wait queue and not before!

    /* create request */
    blob_req = new PutBlobReq(volid,
                              ObjKey,
                              startByte,
                              buflen,
                              buffer,
                              lastBuf,
                              bucket_ctxt,
                              put_properties,
                              req_context,
                              putObjHandler,
                              callback_data);

    if (!blob_req) {
        (putObjHandler)(req_context, 0, 0, NULL, callback_data, FDSN_StatusOutOfMemory, NULL); //NOLINT
        LOGERROR << "FDS_NativeAPI::PutObject bucket "
                 << bucket_ctxt->bucketName
                 << " objKey " << ObjKey
                 << " -- failed to allocate PutBlobReq";
        return;
    }

    if (volid != invalid_vol_id) {
        /* bucket is already attached to this AM, enqueue IO */
        storHvisor->pushBlobReq(blob_req);
        return;  // we are done
    } else {
        /* we are waiting for OM to tell us if bucket exists */
        storHvisor->vol_table->addBlobToWaitQueue(bucket_ctxt->bucketName, blob_req);
    }

    // if we are here, bucket is not attached to this AM, send test bucket msg to OM
    err = sendTestBucketToOM(bucket_ctxt->bucketName,
                             bucket_ctxt->accessKeyId,
                             bucket_ctxt->secretAccessKey);
    if ( !err.ok() ) {
        /* we failed to send query to OM */
        LOGERROR << "FDS_NativeAPI::PutObject bucket "
                 << bucket_ctxt->bucketName
                 << " objKey " << ObjKey
                 << " -- could't find out from OM if bucket exists";
        // remove blob from wait queue (this will also call the callback)
        storHvisor->vol_table->completeWaitBlobsWithError(bucket_ctxt->bucketName, err);
    }
}

void FDS_NativeAPI::DeleteObject(BucketContext *bucket_ctxt,
                                 std::string ObjKey,
                                 void *req_context,
                                 fdsnResponseHandler responseHandler,
                                 void *callback_data)
{
    Error err(ERR_OK);
    fds_volid_t volid = invalid_vol_id;
    FdsBlobReq *blob_req = NULL;
    LOGNORMAL << "FDS_NativeAPI::DeleteObject bucket " << bucket_ctxt->bucketName
              << " objKey " << ObjKey;

    // check if bucket is attached to this AM
    if (storHvisor->vol_table->volumeExists(bucket_ctxt->bucketName)) {
        volid = storHvisor->vol_table->getVolumeUUID(bucket_ctxt->bucketName);
        fds_verify(volid != invalid_vol_id);
    }

    // if bucket is not attached to this AM, before sending attach request to OM
    // create Delete request and put it to wait queue, to make sure that attach arrives
    // after we put a request to the wait queue and not before!

    /* create request */
    blob_req = new DeleteBlobReq(volid,
                                 ObjKey,
                                 bucket_ctxt,
                                 req_context,
                                 responseHandler,
                                 callback_data);

    if (!blob_req) {
        (responseHandler)(FDSN_StatusOutOfMemory, NULL, callback_data);
        LOGERROR << "FDS_NativeAPI::DeleteObject bucket "
                 << bucket_ctxt->bucketName
                 << " objKey " << ObjKey
                 << " -- failed to allocate DeleteBlobReq";
        return;
    }


    if (volid != invalid_vol_id) {
        /* bucket is already attached to this AM, enqueue IO */
        storHvisor->pushBlobReq(blob_req);
        return;
    } else {
        /* we are waiting for OM to tell us if bucket exists */
        storHvisor->vol_table->addBlobToWaitQueue(bucket_ctxt->bucketName, blob_req);
    }

    // if we are here, bucket is not attached to this AM, send test bucket msg to OM
    err = sendTestBucketToOM(bucket_ctxt->bucketName,
                             bucket_ctxt->accessKeyId,
                             bucket_ctxt->secretAccessKey);
    if ( !err.ok() ) {
        /* we failed to send query to OM */
        LOGWARN << "FDS_NativeAPI::DeleteObject bucket " << bucket_ctxt->bucketName
                << " objKey " << ObjKey
                << " -- could't find out from OM if bucket exists";
        // remove blob from wait queue (this will also call the callback)
        storHvisor->vol_table->completeWaitBlobsWithError(bucket_ctxt->bucketName, err);
    }
}

void FDS_NativeAPI::DoCallback(FdsBlobReq  *blob_req,
                               Error        error,
                               fds_uint32_t ignore,
                               fds_int32_t  result)
{
    FDSN_Status status(FDSN_StatusOK);

    LOGNORMAL << " callback -"
              << " [iotype:" << blob_req->getIoType() << "]"
              << " [error:"  << error << "]"
              << " [result:" << result << ":" << static_cast<FDSN_Status>(result) << "]";

    if ( !error.ok() || (result != 0) ) {
        if (result > 0 && result < FDSN_StatusErrorUnknown) {
            // the result is a fdsn status
            status = (FDSN_Status)result;
        } else if (result < 0) {
            // TODO(Prem/Andrew): We're not actually ever setting
            // the error variable and there are cases where AM returns
            // just a negative result value with no other info...we need
            // to either set the error value or remove it and always return
            // the FDSN error code.
            // For now, we just throw our hands up and give a 500.
            status = FDSN_StatusInternalError;
        } else {
            switch (error.GetErrno()) {
                case ERR_OK                           : break;

                case ERR_DUPLICATE                    :
                case ERR_HASH_COLLISION               :
                case ERR_DISK_WRITE_FAILED            :
                case ERR_DISK_READ_FAILED             :
                case ERR_CAT_QUERY_FAILED             :
                case ERR_CAT_ENTRY_NOT_FOUND          :
                case ERR_INVALID_ARG                  :
                case ERR_PENDING_RESP                 : status = FDSN_StatusInternalError; break; //NOLINT

                case ERR_VOL_ADMISSION_FAILED         :
                case ERR_GET_DLT_FAILED               :
                case ERR_GET_DMT_FAILED               :
                case ERR_NOT_IMPLEMENTED              :
                case ERR_OUT_OF_MEMORY                :
                case ERR_DUPLICATE_UUID               :
                case ERR_TRANS_JOURNAL_OUT_OF_IDS     :
                case ERR_TRANS_JOURNAL_REQUEST_QUEUED :
                case ERR_NODE_NOT_ACTIVE              :

                case ERR_NOT_READY                    :
                case ERR_INVALID_DLT                  :
                case ERR_PERSIST_STATE_MISMATCH       :
                case ERR_EXCEED_MIN_IOPS              : status = FDSN_StatusInternalError; break; //NOLINT

                case ERR_UNAUTH_ACCESS                : status = FDSN_StatusErrorAccessDenied; break; //NOLINT

                case ERR_NOT_FOUND                    :
                case ERR_BLOB_NOT_FOUND               : status = FDSN_StatusEntityDoesNotExist; break; //NOLINT
                case ERR_VOL_NOT_FOUND                : status = FDSN_StatusErrorBucketNotExists; break; //NOLINT
                case ERR_VOL_NOT_EMPTY                : status = FDSN_StatusErrorBucketNotEmpty; break; //NOLINT

                default: status = FDSN_StatusInternalError; break;
            }
        }
    }

    switch (blob_req->getIoType()) {
        case FDS_PUT_BLOB:
            static_cast<PutBlobReq*>(blob_req)->DoCallback(status, NULL);
            break;
        case FDS_GET_BLOB:
            static_cast<GetBlobReq*>(blob_req)->DoCallback(status, NULL);
            break;
        case FDS_DELETE_BLOB:
            static_cast<DeleteBlobReq*>(blob_req)->DoCallback(status, NULL);
            break;
        case FDS_LIST_BUCKET:
            /* in case of list bucket, this method is called only on error */
            fds_verify(!error.ok() || (result != 0));
            static_cast<ListBucketReq*>(blob_req)->DoCallback(0, "", 0, NULL, status, NULL); //NOLINT
            break;
        case FDS_BUCKET_STATS:
            /* in case of get bucket stats, this method is called only on error */
            fds_verify(!error.ok() || (result != 0));
            static_cast<BucketStatsReq*>(blob_req)->DoCallback("", 0, NULL, status, NULL);
            break;
        default:
            break;
    };
}

/*
 * Sends 'test bucket' message to OM. If bucket exists (OM knows about it),
 * OM will attach bucket to this AM. Otherwise, will return error.
 *    ERR_OK if test bucket message was sent to OM and now we are waiting for response
 *    other error code if some error happened before we sent msg to OM
 */
Error FDS_NativeAPI::sendTestBucketToOM(const std::string& bucket_name,
                                        const std::string& access_key_id,
                                        const std::string& secret_access_key) {
    Error err(ERR_OK);
    int om_err = 0;
    LOGNORMAL << "FDS_NativeAPI::testBucketInternal bucket " << bucket_name;

    // send test bucket message to OM
    FDSP_VolumeInfoTypePtr vol_info(new FDSP_VolumeInfoType());
    initVolInfo(vol_info, bucket_name);
    om_err = storHvisor->om_client->testBucket(bucket_name,
                                               vol_info,
                                               true,
                                               access_key_id,
                                               secret_access_key);

    if (om_err != 0) {
        err = Error(ERR_INVALID_ARG);
    }
    return err;
}

void FDS_NativeAPI::StatBlob(const std::string& volumeName, const std::string& blobName,
                             native::StatBlobCallbackPtr cb) {
    fds_volid_t volId = invalid_vol_id;
    LOGNORMAL << " volume: " << volumeName
              << " blobName " << blobName;

    native::ScopedCallBack(cb.get());
    // check if bucket is attached to this AM
    if (storHvisor->vol_table->volumeExists(volumeName)) {
        volId = storHvisor->vol_table->getVolumeUUID(volumeName);
        fds_verify(volId != invalid_vol_id);
    } else {
        LOGERROR << "bucket should be here.. but missing : " << volumeName;
        cb->status = FDSN_StatusErrorBucketNotExists;
        return;
    }

    StorHvVolume* vol = storHvisor->vol_table->getVolume(volId);
    fds_verify(vol != NULL);
    fds_uint64_t blobSize;
    Error err = vol->vol_catalog_cache->getBlobSize(blobName, &blobSize);

    if (err == ERR_OK) {
        cb->status = FDSN_StatusOK;
        cb->blobSize = blobSize;
        LOGDEBUG << " blobsize: " << blobSize
                 << " blobname: " << blobName;
    }
}

}  // namespace fds
