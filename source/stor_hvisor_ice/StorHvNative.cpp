/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <native_api.h>
#include <StorHvisorNet.h>

extern StorHvCtrl *storHvisor;

namespace fds {

FDS_NativeAPI::FDS_NativeAPI(FDSN_ClientType type)
  : clientType(type) 
{
}

FDS_NativeAPI::~FDS_NativeAPI()
{
}

/* Create a bucket */
void FDS_NativeAPI::CreateBucket(BucketContext *bucket_ctx, 
				 CannedAcl  canned_acl,
				 void *req_ctxt,
				 fdsnResponseHandler responseHandler,
				 void *callback_data)
{
  fds_volid_t ret_id;
   // check the bucket is already attached. 
  ret_id = storHvisor->vol_table->getVolumeUUID(bucket_ctx->bucketName);
  if (ret_id != invalid_vol_id) {
       FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::error) << " S3 Bucket Already exsists  BucketID: " << ret_id;

     (responseHandler)(FDSN_StatusErrorBucketAlreadyExists,NULL,callback_data);
     return;
   }

   FDSP_VolumeInfoTypePtr vol_info = new FDSP_VolumeInfoType();
     vol_info->vol_name = std::string(bucket_ctx->bucketName);
     vol_info->tennantId = 0;
     vol_info->localDomainId = 0;
     vol_info->globDomainId = 0;

     vol_info->capacity = (1024*1024*100);
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

   // send the  bucket create request to OM

    storHvisor->om_client->pushCreateBucketToOM(vol_info);

    (responseHandler)(FDSN_StatusOK,NULL,callback_data);
     return;
}


/* Get the bucket contents  or objets belonging to this bucket */
void FDS_NativeAPI::GetBucket(BucketContext *bucketContext,
			      std::string prefix,
			      std::string marker,
			      std::string delimiter,
			      fds_uint32_t maxkeys,
			      void *requestContext,
			      fdsnListBucketHandler handler, 
			      void *callbackData)
{
}
 
void FDS_NativeAPI::DeleteBucket(BucketContext* bucketCtxt,
				 void *requestContext,
				 fdsnResponseHandler handler, 
				 void *callbackData)
{
    fds_volid_t ret_id; 
   // check the bucket is already attached. 
   ret_id = storHvisor->vol_table->getVolumeUUID(bucketCtxt->bucketName);
   if (ret_id != invalid_vol_id) {
       FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::error) << " S3 Bucket  Does not exsists  BucketID: " << ret_id;

     (handler)(FDSN_StatusErrorBucketNotExists,NULL,callbackData);
     return;
   }

   FDSP_DeleteVolTypePtr volData = new FDSP_DeleteVolType();
   volData->vol_name  = std::string(bucketCtxt->bucketName);
   volData->domain_id = 0;
   // send the  bucket create request to OM

   storHvisor->om_client->pushDeleteBucketToOM(volData); 
   /* TBD. Since this one is async call Error  checking involved, need more discussiosn */
   (handler)(FDSN_StatusOK,NULL,callbackData);
    return;
}

void FDS_NativeAPI::GetObject(BucketContext *bucket_ctxt, 
			      std::string ObjKey, 
			      GetConditions *get_cond,
			      fds_uint64_t startByte,
			      fds_uint64_t byteCount,
			      char* buffer,
			      fds_uint64_t buflen,
			      void *req_context,
			      fdsnGetObjectHandler getObjCallback,
			      void *callback_data)
{
  Error err(ERR_OK);
  fds_volid_t volid;
  FdsBlobReq *blob_req = NULL;
  FDS_PLOG(storHvisor->GetLog()) << "FDS_NativeAPI::GetObject bucket " << bucket_ctxt->bucketName
				 << " objKey " << ObjKey;

  /* check if bucket is attached to this AM, if not, ask OM to attach */
  err = checkBucketExists(bucket_ctxt, &volid);
  if ( !err.ok() && (err != Error(ERR_PENDING_RESP)) ) {
    /* bucket not attached and we failed to send query to OM */
    (getObjCallback)(req_context, 0, NULL, callback_data, FDSN_StatusInternalError, NULL);
    FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::warning) << "FDS_NativeAPI::GetObject bucket " << bucket_ctxt->bucketName
							      << " objKey " << ObjKey 
							      << " -- could't find out from OM if bucket exists";
    return;
  }

  /* create request */
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
    (getObjCallback)(req_context, 0, NULL, callback_data, FDSN_StatusOutOfMemory, NULL);
    FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::error) << "FDS_NativeAPI::GetObject bucket " 
							    << bucket_ctxt->bucketName
							    << " objKey " << ObjKey 
							    << " -- failed to allocate GetBlobReq";
    return;
  }

  if ( err.ok() ) {
    /* bucket is already attached to this AM, enqueue IO */
      storHvisor->pushBlobReq(blob_req);
  } 
  else if (err == Error(ERR_PENDING_RESP)) {
    /* we are waiting for OM to tell us if bucket exists */
    storHvisor->vol_table->addBlobToWaitQueue(bucket_ctxt->bucketName, blob_req);
  }
  // else we already handled above

}

void FDS_NativeAPI::PutObject(BucketContext *bucket_ctxt, 
			      std::string ObjKey, 
			      PutProperties *put_properties,
			      void *req_context,
			      char *buffer,
			      fds_uint64_t buflen,
			      fdsnPutObjectHandler putObjHandler, 
			      void *callback_data)
{
  Error err(ERR_OK);
  fds_volid_t volid;
  FdsBlobReq *blob_req = NULL;
  FDS_PLOG(storHvisor->GetLog()) << "FDS_NativeAPI::PutObject bucket " << bucket_ctxt->bucketName
				 << " objKey " << ObjKey;


  /* check if bucket is attached to this AM, if not, ask OM to attach */
  err = checkBucketExists(bucket_ctxt, &volid);
  if ( !err.ok() && (err != Error(ERR_PENDING_RESP)) ) {
    /* bucket not attached and we failed to send query to OM */
    FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::warning) << "FDS_NativeAPI::PutObject bucket " 
							      << bucket_ctxt->bucketName
							      << " objKey " << ObjKey 
							      << " -- could't find out from OM if bucket exists";

    (putObjHandler)(req_context, 0, NULL, callback_data, FDSN_StatusInternalError, NULL);
    return;
  }

  /* create request */
  blob_req = new PutBlobReq(volid, 
			    ObjKey,
			    0,
			    buflen,
			    buffer,
			    bucket_ctxt,
			    put_properties,
			    req_context,
			    putObjHandler,
			    callback_data);

  if (!blob_req) {
    (putObjHandler)(req_context, 0, NULL, callback_data, FDSN_StatusOutOfMemory, NULL);
    FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::error) << "FDS_NativeAPI::PutObject bucket " 
							    << bucket_ctxt->bucketName
							    << " objKey " << ObjKey 
							    << " -- failed to allocate PutBlobReq";
    return;
  }

  if ( err.ok() ) {
    /* bucket is already attached to this AM, enqueue IO */
      storHvisor->pushBlobReq(blob_req);
  } 
  else if (err == Error(ERR_PENDING_RESP)) {
    /* we are waiting for OM to tell us if bucket exists */
    storHvisor->vol_table->addBlobToWaitQueue(bucket_ctxt->bucketName, blob_req);
  }
  // else we already handled above 

}

void FDS_NativeAPI::DeleteObject(BucketContext *bucket_ctxt, 
				 std::string ObjKey,
				 void *req_context,
				 fdsnResponseHandler responseHandler,
				 void *callback_data)
{
  Error err(ERR_OK);
  fds_volid_t volid;
  FdsBlobReq *blob_req = NULL;
  FDS_PLOG(storHvisor->GetLog()) << "FDS_NativeAPI::DeleteObject bucket " << bucket_ctxt->bucketName
				 << " objKey " << ObjKey;

  /* check if bucket is attached to this AM, if not, ask OM to attach */
  err = checkBucketExists(bucket_ctxt, &volid);
  if ( !err.ok() && (err != Error(ERR_PENDING_RESP)) ) {
    /* bucket not attached and we failed to send query to OM */
    FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::warning) << "FDS_NativeAPI::DeleteObject bucket " << bucket_ctxt->bucketName
							      << " objKey " << ObjKey 
							      << " -- could't find out from OM if bucket exists";
    (responseHandler)(FDSN_StatusInternalError, NULL, callback_data);
    return;
  }

  /* create request */
  blob_req = new DeleteBlobReq(volid, 
			       ObjKey,
			       bucket_ctxt,
			       req_context,
			       responseHandler,
			       callback_data);

  if (!blob_req) {
    (responseHandler)(FDSN_StatusOutOfMemory, NULL, callback_data);
    FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::error) << "FDS_NativeAPI::DeleteObject bucket " 
							    << bucket_ctxt->bucketName
							    << " objKey " << ObjKey 
							    << " -- failed to allocate DeleteBlobReq";
    return;
  }


  if ( err.ok() ) {
    /* bucket is already attached to this AM, enqueue IO */
    storHvisor->pushBlobReq(blob_req);
  } 
  else if (err == Error(ERR_PENDING_RESP)) {
    /* we are waiting for OM to tell us if bucket exists */
    storHvisor->vol_table->addBlobToWaitQueue(bucket_ctxt->bucketName, blob_req);
  }
  // else we already handled above 
}

void FDS_NativeAPI::DoCallback(FdsBlobReq* blob_req, Error error)
{
  FDS_PLOG(storHvisor->GetLog()) << "FDS_NativeAPI: callback to complete blob request : "
				 << error.GetErrstr();

  switch (blob_req->getIoType()) {
  case FDS_PUT_BLOB:
    static_cast<PutBlobReq*>(blob_req)->DoCallback(FDSN_StatusInternalError, NULL);
    break;
  case FDS_GET_BLOB:
    break;
  case FDS_DELETE_BLOB:
    break;
  };
}

/*
 * Checks if bucket is attached to AM, if not, sends test bucket message to OM.
 * returns error:
 *    ERR_OK if bucket is already attached to AM
 *    ERR_PENDING_RESP is test bucket message was sent to AM and now we are waiting
 *    for response from OM 
 *    other error code if some error happened
 *
 * \return ret_volid will contain volume id for the bucket if returned value is ERR_OK
 */
Error FDS_NativeAPI::checkBucketExists(BucketContext *bucket_ctxt, fds_volid_t* ret_volid)
{
  Error err(ERR_OK);
  int om_err = 0;
  fds_volid_t volid = invalid_vol_id;
  FDS_PLOG(storHvisor->GetLog()) << "FDS_NativeAPI::testBucketInternal  bucket " << bucket_ctxt->bucketName;

  volid = storHvisor->vol_table->volumeExists(bucket_ctxt->bucketName);
  if (volid != invalid_vol_id) {
    *ret_volid = volid;
    return err; /* success */
  }

  /* else -- the volume not attached but it could have been already created,
   * so we will send test bucket msg to OM */
  FDSP_VolumeInfoTypePtr vol_info = new FDSP_VolumeInfoType();
  vol_info->vol_name = std::string(bucket_ctxt->bucketName);
  vol_info->volType = FDSP_VOL_S3_TYPE;
  /* at this point we don't know anything about volume (?) */

  om_err = storHvisor->om_client->testBucket(bucket_ctxt->bucketName,
					     vol_info,
					     true,
					     bucket_ctxt->accessKeyId,
					     bucket_ctxt->secretAccessKey);

  err = (om_err == 0) ? Error(ERR_PENDING_RESP) : Error(ERR_INVALID_ARG); //TODO: we need some generic error

  *ret_volid = volid;
  return err;
}


} // namespace fds
