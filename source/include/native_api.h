#ifndef SOURCE_FDS_NATIVE_API_H_
#define SOURCE_FDS_NATIVE_API_H_

#include <fds_types.h>
#include <fds_error.h>
#include <list>

/* Put Bucket Policy and Get Stats use normalized values (between 0 and 100) for 
 * "sla" (iops_min), "limit" (iops_max) and "performance" (average iops). 
 * For now we are just dividing absolute values by the below constant number 
 * -- I got it from the maximum end-to-end performance we are getting right now 
 * = 3200 IOPS divided by 100 to get % when we divide sla/perf/limit by this number 
 * We will just tune this number for now, and dicide later how to better normalize 
 * these values. */
#define FDSN_QOS_PERF_NORMALIZER 20 

namespace fds { 
class BucketContext {
  public:
    std::string    hostName;
    std::string   bucketName;
    std::string   accessKeyId; // Identifies the tennantID or accountName
    std::string   secretAccessKey;

    BucketContext(const std::string& _hostName,
                  const std::string& _bucketName,
                  const std::string& _accessKeyId,
		const std::string& _secretAccessKey) 
            : hostName(_hostName),
              bucketName(_bucketName),
              accessKeyId(_accessKeyId),
              secretAccessKey(_secretAccessKey)
    {
    }
    ~BucketContext() {}
};
/// Smart pointer for BucketContext
typedef boost::shared_ptr<BucketContext> BucketContextPtr;

class MetaNameValue { 

public:
  std::string   Name;
  std::string   Value;
  MetaNameValue();
  ~MetaNameValue();
};

class QosParams {
 public:
  double iops_min;
  double iops_max;
  int relativePrio;

  QosParams(double _iops_min,
	    double _iops_max,
	    int _prio)
    : iops_min(_iops_min),
    iops_max(_iops_max),
    relativePrio(_prio) {
    }
  ~QosParams() {}
};

class ListBucketContents {
public:
   std::string objKey;
   fds_int64_t  lastModified; // Seconds since last modified
   std::string  eTag; // This is similar to MD5
   fds_uint64_t size;
   std::string  ownerId;
   std::string  ownerDisplayName;

   ListBucketContents() {}
   ~ListBucketContents() {}

   void set(const std::string& _key, 
	    fds_uint64_t _modified,
	    const std::string& _eTag,
	    fds_uint64_t _size,
	    const std::string& _ownerId,
	    const std::string& _ownerDisplayName)
   {
     objKey = _key;
     lastModified = _modified;
     eTag = _eTag;
     size = _size;
     ownerId = _ownerId;
     ownerDisplayName = _ownerDisplayName;
   }
};

class BucketStatsContent {
public:
  std::string bucket_name; /* bucket name */
  int priority;            /* bucket's priority */
  double performance;      /* bucket's average iops (timer interval configurable) */
  double sla;              /* bucket's minimum iops */
  double limit;            /* bucket's maximum iops */

  BucketStatsContent() 
    : bucket_name(""),
    priority(0),
    performance(0),
    sla(0),
    limit(0) {
    }

  ~BucketStatsContent() {}

  void set(const std::string& _name,
	   int _prio,
	   double _perf,
	   double _sla,
	   double _limit) {
    bucket_name = _name;
    priority = _prio;

    /* we are currently returning values from 0 to 100, 
     * so normalize the values */
    performance = _perf / (double)FDSN_QOS_PERF_NORMALIZER;
    if (performance > 100.0) {
      performance = 100.0;
    }
    sla = _sla / (double)FDSN_QOS_PERF_NORMALIZER;
    if (sla > 100.0) {
      sla = 100.0;
    }
    limit = _limit / (double)FDSN_QOS_PERF_NORMALIZER;
    if (limit > 100.0) {
      limit = 100.0;
    }
  }
};

typedef enum
{
    AclPermissionRead                    = 0,
    AclPermissionWrite                   = 1,
    AclPermissionReadACP                 = 2,
    AclPermissionWriteACP                = 3,
    AclPermissionFullControl             = 4
} AclPermission;

typedef struct ErrorDetails
{
    /**
     * This is the human-readable message that Amazon supplied describing the
     * error
     **/
    const char *message;

    /**
     * This identifies the resource for which the error occurred
     **/
    const char *resource;

    /**
     * This gives human-readable further details describing the specifics of
     * this error
     **/
    const char *furtherDetails;

    /**
     * This gives the number of S3NameValue pairs present in the extraDetails
     * array
     **/
    int extraDetailsCount;

    /**
     * S3 can provide extra details in a freeform Name - Value pair format.
     * Each error can have any number of these, and this array provides these
     * additional extra details.
     **/
    MetaNameValue *extraDetails;
} ErrorDetails;

/**
 * S3CannedAcl is an ACL that can be specified when an object is created or
 * updated.  Each canned ACL has a predefined value when expanded to a full
 * set of S3 ACL Grants.
 * Private canned ACL gives the owner FULL_CONTROL and no other permissions
 *     are issued
 * Public Read canned ACL gives the owner FULL_CONTROL and all users Read
 *     permission
 * Public Read Write canned ACL gives the owner FULL_CONTROL and all users
 *     Read and Write permission
 * AuthenticatedRead canned ACL gives the owner FULL_CONTROL and authenticated
 *     S3 users Read permission
 **/
typedef enum
{
    CannedAclPrivate                  = 0, /* private */
    CannedAclPublicRead               = 1, /* public-read */
    CannedAclPublicReadWrite          = 2, /* public-read-write */
    CannedAclAuthenticatedRead        = 3  /* authenticated-read */
} CannedAcl;


typedef enum
{
    GranteeTypeAmazonCustomerByEmail  = 0,
    GranteeTypeCanonicalUser          = 1,
    GranteeTypeAllAwsUsers            = 2,
    GranteeTypeAllUsers               = 3,
    GranteeTypeLogDelivery            = 4
} GranteeType;

class AclGrant {
    GranteeType granteeType;
    std::string emailAddress;
    std::string canonicalUserId;
    std::string displayName;
    AclPermission permission;
};


class PutProperties {
  public:
    /**
     * If present, this is the Content-Type that should be associated with the
     * object.  If not provided, S3 defaults to "binary/octet-stream".
     **/
    std::string contentType;

    /**
     * If present, this provides the MD5 signature of the contents, and is
     * used to validate the contents.  This is highly recommended by Amazon
     * but not required.  Its format is as a base64-encoded MD5 sum.
     **/
    std::string  md5;

    /**
     * If present, this gives a Cache-Control header string to be supplied to
     * HTTP clients which download this
     **/
    std::string cacheControl;

    /**
     * If present, this gives the filename to save the downloaded file to,
     * whenever the object is downloaded via a web browser.  This is only
     * relevent for objects which are intended to be shared to users via web
     * browsers and which is additionally intended to be downloaded rather
     * than viewed.
     **/
    std::string contentDispositionFilename;

    /**
     * If present, this identifies the content encoding of the object.  This
     * is only applicable to encoded (usually, compressed) content, and only
     * relevent if the object is intended to be downloaded via a browser.
     **/
    std::string contentEncoding;

    /**
     * If >= 0, this gives an expiration date for the content.  This
     * information is typically only delivered to users who download the
     * content via a web browser.
     **/
    fds_int64_t expires;
        /**
     * This identifies the "canned ACL" that should be used for this object.
     * The default (0) gives only the owner of the object access to it.
     **/
    CannedAcl cannedAcl;

    /**
     * This is the number of values in the metaData field.
     **/
    int metaDataCount;

    /**
     * These are the meta data to pass to S3.  In each case, the name part of
     * the Name - Value pair should not include any special S3 HTTP header
     * prefix (i.e., should be of the form 'foo', NOT 'x-amz-meta-foo').
     **/
    const MetaNameValue *metaData;
};
typedef boost::shared_ptr<PutProperties> PutPropertiesPtr;

// If the ifModifiedSince and ifNotModifiedSince are invalid if negative
//If ifMatchETag or ifNotMatchETag is non-null then they are valid 
class GetConditions {
  fds_int64_t   ifModifiedSince;
  fds_int64_t   ifNotModifiedSince;
  std::string    ifMatchETag;
  std::string    ifNotMatchETag;
};


/**
 * This callback is made during a put object operation, to obtain the next
 * chunk of data to put to the FDS system as the contents of the object.  This
 * callback is made repeatedly, each time acquiring the next chunk of data to
 * write to the service, until a negative or 0 value is returned.
 *
 * @param bufferSize gives the maximum number of bytes that may be written
 *        into the buffer parameter by this callback
 * @param buffer gives the buffer to fill with at most bufferSize bytes of
 *        data as the next chunk of data to send to S3 as the contents of this
 *        object
 * @param offset of the buffer that was put
 * @param callbackData is the callback data as specified when the request
 *        was issued.
 * @return < 0 to abort the request with the S3StatusAbortedByCallback, which
 *        will be pased to the response complete callback for this request, or
 *        0 to indicate the end of data, or > 0 to identify the number of
 *        bytes that were written into the buffer by this callback
 **/
typedef int (*fdsnPutObjectHandler)(void *reqContext, fds_uint64_t bufferSize,
                                    fds_off_t offset, char *buffer, 
				    void *callbackData, FDSN_Status status, ErrorDetails* errDetails);


/**
 * This callback is made repeatedly, each time
 * providing the next chunk of data read, until the complete object contents
 * have been passed through the callback in this way, or the callback
 * returns an error status.
 *
 * @param bufferSize gives the number of bytes in buffer
 * @param buffer is the data being passed into the callback
 * @param blobSize returned size of the blob we read from
 * @param callbackData is the callback data as specified when the request
 *        was issued.
 * @return StatusOK to continue processing the request, anything else to
 *         immediately abort the request with a status which will be
 *         passed to the S3ResponseCompleteCallback for this request.
 *         Typically, this will return either S3StatusOK or
 *         S3StatusAbortedByCallback.
 **/
typedef FDSN_Status (*fdsnGetObjectHandler)(BucketContextPtr bucket_ctx,
                                            void *reqContext,
                                            fds_uint64_t bufferSize,
                                            fds_off_t offset,
                                            const char *buffer,
                                            fds_uint64_t blobSize,
                                            const std::string &blobEtag,
                                            void *callbackData,
                                            FDSN_Status status,
                                            ErrorDetails *errDetails);
typedef void (*fdsnResponseHandler)(FDSN_Status status,
                                          const ErrorDetails *errorDetails,
                                          void *callbackData);
typedef void (*fdsnListBucketHandler)(int isTruncated,
				      const char *nextMarker,
				      int contentsCount,
				      const ListBucketContents *contents,
				      int commonPrefixesCount,
				      const char **commonPrefixes,
				      void *callbackData,
				      FDSN_Status status);

typedef void (*fdsnBucketStatsHandler)(const std::string& timestamp,
				       int content_count,
				       const BucketStatsContent* contents,
				       void *req_context,
				       void *callbackData,
				       FDSN_Status status,
				       ErrorDetails *err_details);


// FDS_NativeAPI  object class : One object per client Type so that the semantics of 
// the particular access protocols can be followed in returning the data
// TODO: [Vy] I'd like to make this class as singleton and inherits from
// fds::Module class.
//
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

    // TODO: [Vy]: I think this API layer doesn't need to aware of any client's
    // semantics.  Specific semantic is handled by the connector layer.
    //
    FDS_NativeAPI(FDSN_ClientType type);
    ~FDS_NativeAPI(); 
    typedef boost::shared_ptr<FDS_NativeAPI> ptr;

    // Create a bucket
    void CreateBucket(BucketContext *bucket_ctx, CannedAcl  canned_acl, 
                      void *req_ctxt, fdsnResponseHandler responseHandler, void *callback_data);

    // Get the bucket contents  or objets belonging to this bucket
    void GetBucket(BucketContext *bucket_ctxt,
                   std::string prefix, std::string marker,
                   std::string delimiter, fds_uint32_t maxkeys,
                   void *requestContext,
                   fdsnListBucketHandler handler, void *callbackData);

    void DeleteBucket(BucketContext *bucketCtxt,
                      void *requestContext,
                      fdsnResponseHandler handler, void *callbackData);

    /* Modify bucket policy */
    void ModifyBucket(BucketContext *bucket_ctxt,
                      const QosParams& qos_params,
                      void* req_ctxt,
                      fdsnResponseHandler handler,
                      void *callback_data);

    /* Retreive stats of all existing buckets
     * Buckets do not need to be attached to this AM, retrieves stats directly from OM 
     * Note: in the future, we can also have API to get stats for a particular bucket */
    void GetBucketStats(void *req_ctxt,
                        fdsnBucketStatsHandler resp_handler,
                        void *callback_data);

    // After this call returns bucketctx, get_cond are no longer valid.
    void GetObject(BucketContextPtr bucketctxt, 
                   std::string ObjKey, 
                   GetConditions *get_cond, 
                   fds_uint64_t startByte, 
                   fds_uint64_t byteCount,
                   char *buffer, 
                   fds_uint64_t buflen,
                   void *reqcontext,
                   fdsnGetObjectHandler getObjCallback,
                   void *callbackdata);

    void PutObject(BucketContext *bucket_ctxt, 
                   std::string ObjKey, 
                   PutPropertiesPtr putproperties,
                   void *reqContext,
                   char *buffer, 
                   fds_uint64_t startByte, 
                   fds_uint64_t buflen,
                   fds_bool_t lastBuf,
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
                             const std::string& access_key_id,
                             const std::string& secret_access_key);

    /* helper function to initialize volume info to some default values, used by several native api methods */
    void initVolInfo(FDS_ProtocolInterface::FDSP_VolumeInfoTypePtr vol_info, 
                     const std::string& bucket_name);

    void initVolDesc(FDS_ProtocolInterface::FDSP_VolumeDescTypePtr vol_desc, 
                     const std::string& bucket_name);

};
}  // namespace fds
#endif 
