/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NATIVE_API_H_
#define SOURCE_INCLUDE_NATIVE_API_H_

#include <fds_types.h>
#include <fds_error.h>
#include <map>
#include <list>
#include <native/types.h>
#include <string>
#include <fds_typedefs.h>

/**
 * Put Bucket Policy and Get Stats use normalized values (between 0 and 100) for
 * "sla" (iops_min), "limit" (iops_max) and "performance" (average iops).
 * For now we are just dividing absolute values by the below constant number
 * -- I got it from the maximum end-to-end performance we are getting right now
 * = 3200 IOPS divided by 100 to get % when we divide sla/perf/limit by this number
 * We will just tune this number for now, and dicide later how to better normalize
 * these values.
 */
namespace fds {

struct FdsBlobReq;

/**
 * FDS_NativeAPI  object class : One object per client Type so that the semantics of
 * the particular access protocols can be followed in returning the data
 * TODO:(Vy):  I'd like to make this class as singleton and inherits from
 * fds::Module class.
 *
 */
class FDS_NativeAPI {
  public:
    enum FDSN_ClientType {
        FDSN_AWS_S3,
        FDSN_MSFT_AZURE,
        FDSN_OPEN_STACK_SWIFT,
        FDSN_NATIVE_OBJ_API,
        FDSN_BLOCK_DEVICE,
        FDSN_EMC_ATMOS,
        FDSN_CLIENT_TYPE_MAX
    };
    FDSN_ClientType clientType;

    /// TODO: [Vy]: I think this API layer doesn't need to aware of any client's
    /// semantics.  Specific semantic is handled by the connector layer.
    ///
    explicit FDS_NativeAPI(FDSN_ClientType type);
    ~FDS_NativeAPI();
    typedef boost::shared_ptr<FDS_NativeAPI> ptr;

    /// Create a bucket
    void CreateBucket(BucketContext *bucket_ctx, CannedAcl  canned_acl,
                      void *req_ctxt, fdsnResponseHandler responseHandler, void *callback_data);

    /// Get the bucket contents  or objets belonging to this bucket
    void GetBucket(BucketContext *bucket_ctxt,
                   std::string prefix, std::string marker,
                   std::string delimiter, fds_uint32_t maxkeys,
                   void *requestContext,
                   fdsnVolumeContentsHandler handler, void *callbackData);

    /// Delete a bucket
    void DeleteBucket(BucketContext *bucketCtxt,
                      void *requestContext,
                      fdsnResponseHandler handler, void *callbackData);

    /// Modify bucket policy
    void ModifyBucket(BucketContext *bucket_ctxt,
                      const QosParams& qos_params,
                      void* req_ctxt,
                      fdsnResponseHandler handler,
                      void *callback_data);

    /**
     * Retreive stats of all existing buckets
     * Buckets do not need to be attached to this AM, retrieves stats directly from OM
     * Note: in the future, we can also have API to get stats for a particular bucket
     */
    void GetVolumeStats(void *req_ctxt,
                        fdsnVolumeStatsHandler resp_handler,
                        void *callback_data);

    /// After this call returns bucketctx, get_cond are no longer valid.
    void GetObject(BucketContextPtr bucketctxt,
                   const std::string &blobName,
                   GetConditions *get_cond,
                   fds_uint64_t startByte,
                   fds_uint64_t byteCount,
                   char *buffer,
                   fds_uint64_t buflen,
                   CallbackPtr cb);

    void PutBlob(BucketContext *bucket_ctxt,
                 std::string ObjKey,
                 PutPropertiesPtr putproperties,
                 void *reqContext,
                 char *buffer,
                 fds_uint64_t startByte,
                 fds_uint64_t buflen,
                 BlobTxId::ptr txDesc,
                 fds_bool_t lastBuf,
                 fdsnPutObjectHandler putObjHandler,
                 void *callbackData);

    /**
     * Performs a putBlob operation in a single call. No
     * prior transaction context or commit request is needed.
     * If success if returned, the update has been made and
     * committed to the blob.
     */
    void PutBlobOnce(const std::string& volumeName,
                     const std::string& blobName,
                     char *buffer,
                     fds_uint64_t startByte,
                     fds_uint64_t buflen,
                     fds_int32_t blobMode,
                     boost::shared_ptr< std::map<std::string, std::string> >& metadata,
                     fdsnPutObjectHandler putObjHandler,
                     void *callbackData);

    void DeleteObject(BucketContext *bucket_ctxt,
                      std::string ObjKey,
                      void *reqcontext,
                      fdsnResponseHandler responseHandler,
                      void *callbackData);

    static void DoCallback(FdsBlobReq* blob_req,
                           Error error,
                           fds_uint32_t ignore,
                           fds_int32_t  result);

    // Functions below use callbacks on the newer scheme

    void StatBlob(const std::string& volumeName,
                  const std::string& blobName,
                  CallbackPtr cb);

    void StartBlobTx(const std::string& volumeName,
                     const std::string& blobName,
                     const fds_int32_t blobMode,
                     CallbackPtr cb);
    void CommitBlobTx(const std::string& volumeName,
                     const std::string& blobName,
                     BlobTxId::ptr txDesc,
                     CallbackPtr cb);

    void AbortBlobTx(const std::string& volumeName,
                     const std::string& blobName,
                     BlobTxId::ptr txDesc,
                     CallbackPtr cb);

    void attachVolume(const std::string& volumeName,
                      CallbackPtr cb);

    void setBlobMetaData(const std::string& volumeName,
                         const std::string& blobName,
                         BlobTxId::ptr txDesc,
                         boost::shared_ptr<fpi::FDSP_MetaDataList>& metaDataList,
                         CallbackPtr cb);

    void GetVolumeMetaData(const std::string& volumeName, CallbackPtr cb);

  private:
    /**
     * Sends 'test bucket' message to OM. If bucket exists (OM knows about it),
     * OM will attach the bucket to this AM. Otherwise, will return error.
     * return error:
     *    ERR_OK if test bucket message was sent to OM and now we are waiting
     *    for attach bucket from AM
     *    Other error codes if error happened.
     */
    Error sendTestBucketToOM(const std::string& bucket_name,
                             const std::string& access_key_id = "" ,
                             const std::string& secret_access_key = "");

    /**
     * Helper function to initialize volume info to some default values, 
     * used by several native api methods
     */
    void initVolInfo(FDS_ProtocolInterface::FDSP_VolumeInfoTypePtr vol_info,
                     const std::string& bucket_name);

    void initVolDesc(FDS_ProtocolInterface::FDSP_VolumeDescTypePtr vol_desc,
                     const std::string& bucket_name);
};
}  // namespace fds
#endif  // SOURCE_INCLUDE_NATIVE_API_H_
