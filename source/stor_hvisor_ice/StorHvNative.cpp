/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <native_api.h>
#include <StorHvisorNet.h>

extern StorHvCtrl *storHvisor;

namespace fds {

FDS_NativeAPI::FDS_NativeAPI(FDSN_ClientType type)
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
			      void *reqcontext,
			      fdsnGetObjectHandler getObjCallback,
			      void *callbackdata)
{
}

void FDS_NativeAPI::PutObject(BucketContext *buckctxt, 
			      std::string ObjKey, 
			      PutProperties *putproperties,
			      void *reqContext,
			      fdsnPutObjectHandler putObjHandler, 
			      void *callbackData)
{
}

void FDS_NativeAPI::DeleteObject(BucketContext *bucket_ctxt, 
				 std::string ObjKey,
				 void *reqcontext,
				 fdsnResponseHandler responseHandler,
				 void *callbackData)
{
}

} // namespace fds
