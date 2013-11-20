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
 
void FDS_NativeAPI::DeleteBucket(BucketContext bucketCtxt,
				 void *requestContext,
				 fdsnResponseHandler *handler, 
				 void *callbackData)
{
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
