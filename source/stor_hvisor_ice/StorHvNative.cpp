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
                  Conn_GetObject *connector)
{
    int   get_len, got_len;
    void  *cookie;
    char  *buf;
    // Process the request

    // Now send the header response.
    get_len = 100;
    connector->fdsn_send_get_response(0, get_len);

    // Now send the data response.
    cookie = connector->fdsn_alloc_get_buffer(get_len, &buf, &got_len);

    // Assuming get_len == got_len always for now.
    // Fill in the data to the buf.

    connector->fdsn_send_get_buffer(cookie, get_len, true);
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
