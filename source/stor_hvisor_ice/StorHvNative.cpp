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
   ret_id = storHvisor->vol_table->volumeExists(bucket_ctx->bucketName);
   if (ret_id == invalid_vol_id) {
       FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::error) << " S3 Bucket Already exsists  BucketID: " << ret_id;

     (responseHandler)(FDSN_StatusErrorBucketAlreadyExists,NULL,NULL);
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

    (responseHandler)(FDSN_StatusOK,NULL,NULL);
     return;
}


/* Get the bucket contents  or objets belonging to this bucket */
void FDS_NativeAPI::GetBucket(BucketContext *bucketContext,
			      std::string prefix,
			      std::string marker,
			      std::string delimiter,
			      fds_uint32_t maxkeys,
			      void *requestContext,
			      fdsnListBucketHandler *handler, 
			      void *callbackData)
{
}
 
void FDS_NativeAPI::DeleteBucket(BucketContext* bucketCtxt,
				 void *requestContext,
				 fdsnResponseHandler *handler, 
				 void *callbackData)
{
    fds_volid_t ret_id; 
   // check the bucket is already attached. 
   ret_id = storHvisor->vol_table->volumeExists(bucketCtxt->bucketName);
   if (ret_id == invalid_vol_id) {
       FDS_PLOG_SEV(storHvisor->GetLog(), fds::fds_log::error) << " S3 Bucket  Does not exsists  BucketID: " << ret_id;

     (handler)(FDSN_StatusErrorBucketNotExists,NULL,NULL);
     return;
   }

   FDSP_DeleteVolTypePtr volData = new FDSP_DeleteVolType();
   volData->vol_name  = std::string(bucketCtxt->bucketName);
   volData->domain_id = 0;
   // send the  bucket create request to OM

   storHvisor->om_client->pushDeleteBucketToOM(volData); 
   /* TBD. Since this one is async call Error  checking involved, need more discussiosn */
   (handler)(FDSN_StatusOK,NULL,NULL);
    return;
}

void FDS_NativeAPI::GetObject(BucketContext *bucketctxt, 
			      std::string ObjKey, 
			      GetConditions *get_cond,
			      fds_uint64_t startByte,
			      fds_uint64_t byteCount,
			      char* buffer,
			      fds_uint64_t buflen,
			      void *reqcontext,
			      fdsnGetObjectHandler getObjCallback,
			      void *callbackdata)
{
  FDS_PLOG(storHvisor->GetLog()) << "FDS_NativeAPI::GetObject bucket " << bucketctxt->bucketName
				 << " objKey " << ObjKey;

  /* check if bucket is attached to this AM, if not, ask OM to attach */
  /* TBD */

}

void FDS_NativeAPI::PutObject(BucketContext *bucket_ctxt, 
			      std::string ObjKey, 
			      PutProperties *putproperties,
			      void *reqContext,
			      char *buffer, 
			      fds_uint64_t buflen,
			      fdsnPutObjectHandler putObjHandler, 
			      void *callbackData)
{
  Error err(ERR_OK);
  FDS_PLOG(storHvisor->GetLog()) << "FDS_NativeAPI::PutObject bucket " << bucket_ctxt->bucketName
				 << " objKey " << ObjKey;

  /* first create fds io request */

  /* TBD: see if we can move code below to common method put/get/delete can reuse */

  /* check if bucket is attached to this AM, if not, ask OM to attach */
  err = testBucketInternal(bucket_ctxt);
  if ( err.ok() ) {
    /* bucket is already attached to this AM, enqueue IO */
  } 
  else if (err == ERR_PENDING_RESP) {
    /* we are waiting for OM to tell us if bucket exists */
  }
  else {
    /* respond right away with an error */
  }

}

void FDS_NativeAPI::DeleteObject(BucketContext *bucket_ctxt, 
				 std::string ObjKey,
				 void *reqcontext,
				 fdsnResponseHandler responseHandler,
				 void *callbackData)
{
  FDS_PLOG(storHvisor->GetLog()) << "FDS_NativeAPI::DeleteObject bucket " << bucket_ctxt->bucketName
				 << " objKey " << ObjKey;

  /* check if bucket is attached to this AM, if not, ask OM to attach */
  /* TBD */
}

/*
 * Checks if bucket is attached to AM, if not, sends test bucket message to OM.
 * returns error:
 *    ERR_OK if bucket is already attached to AM
 *    ERR_PENDING_RESP is test bucket message was sent to AM and now we are waiting
 *    for response from OM 
 *    other error code if some error happened
 */
Error FDS_NativeAPI::testBucketInternal(BucketContext *bucket_ctxt)
{
  Error err(ERR_OK);
  FDS_PLOG(storHvisor->GetLog()) << "FDS_NativeAPI::testBucketInternal  bucket " << bucket_ctxt->bucketName;

  if (storHvisor->vol_table->volumeExists(bucket_ctxt->bucketName)) {
    return err; /* success */
  }

  /* else -- the volume not attached but it could have been already created,
   * so we will send test bucket msg to OM */

  return err;
}


} // namespace fds
