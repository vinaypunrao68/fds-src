#ifndef SOURCE_FDS_NATIVE_API_H_
#define SOURCE_FDS_NATIVE_API_H_

/* Only include what we need to avoid un-needed dependencies. */
/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <fdsp/FDSP.h>
#include <fds_volume.h>
*/
#include <fds_types.h>
#include <fds_err.h>
/*
#include <unistd.h>
#include <assert.h>
#include <iostream>
#include <Ice/Ice.h>
#include <util/Log.h>
#include <util/Log.h>
#include <concurrency/Mutex.h>
#include <concurrency/RwLock.h>

#include <utility>
#include <atomic>
*/
#include <am-engine/am-engine.h>
#include <list>

namespace fds { 
class BucketContext { 
public:
  std::string    hostName;
  std::string   bucketName;
  std::string   accessKeyId; // Identifies the tennantID or accountName
  std::string   secretAccessKey;

  BucketContext();
  ~BucketContext();
};

class MetaNameValue { 

public:
  std::string   Name;
  std::string   Value;
  MetaNameValue();
  ~MetaNameValue();
};

typedef enum
{
    FDSN_StatusOK                                              ,

    /**
     * Errors that prevent the S3 request from being issued or response from
     * being read
     **/
    FDSN_StatusInternalError                                   ,
    FDSN_StatusOutOfMemory                                     ,
    FDSN_StatusInterrupted                                     ,
    FDSN_StatusInvalidBucketNameTooLong                        ,
    FDSN_StatusInvalidBucketNameFirstCharacter                 ,
    FDSN_StatusInvalidBucketNameCharacter                      ,
    FDSN_StatusInvalidBucketNameCharacterSequence              ,
    FDSN_StatusInvalidBucketNameTooShort                       ,
    FDSN_StatusInvalidBucketNameDotQuadNotation                ,
    FDSN_StatusQueryParamsTooLong                              ,
    FDSN_StatusFailedToInitializeRequest                       ,
    FDSN_StatusMetaDataHeadersTooLong                          ,
    FDSN_StatusBadMetaData                                     ,
    FDSN_StatusBadContentType                                  ,

    FDSN_StatusContentTypeTooLong                              ,
    FDSN_StatusBadMD5                                          ,
    FDSN_StatusMD5TooLong                                      ,
    FDSN_StatusBadCacheControl                                 ,
    FDSN_StatusCacheControlTooLong                             ,
    FDSN_StatusBadContentDispositionFilename                   ,
    FDSN_StatusContentDispositionFilenameTooLong               ,
    FDSN_StatusBadContentEncoding                              ,
    FDSN_StatusContentEncodingTooLong                          ,
    FDSN_StatusBadIfMatchETag                                  ,
    FDSN_StatusIfMatchETagTooLong                              ,
    FDSN_StatusBadIfNotMatchETag                               ,
    FDSN_StatusIfNotMatchETagTooLong                           ,
    FDSN_StatusHeadersTooLong                                  ,
    FDSN_StatusKeyTooLong                                      ,
    FDSN_StatusUriTooLong                                      ,
    FDSN_StatusXmlParseFailure                                 ,
    FDSN_StatusEmailAddressTooLong                             ,
    FDSN_StatusUserIdTooLong                                   ,
    FDSN_StatusUserDisplayNameTooLong                          ,
    FDSN_StatusGroupUriTooLong                                 ,
    FDSN_StatusPermissionTooLong                               ,
    FDSN_StatusTargetBucketTooLong                             ,
    FDSN_StatusTargetPrefixTooLong                             ,
    FDSN_StatusTooManyGrants                                   ,
    FDSN_StatusBadGrantee                                      ,
    FDSN_StatusBadPermission                                   ,
    FDSN_StatusXmlDocumentTooLarge                             ,
    FDSN_StatusNameLookupError                                 ,
    FDSN_StatusFailedToConnect                                 ,
    FDSN_StatusServerFailedVerification                        ,
    FDSN_StatusConnectionFailed                                ,
    FDSN_StatusAbortedByCallback                               ,

    /**
     * Errors from the S3 service
     **/
    FDSN_StatusErrorAccessDenied                               ,
    FDSN_StatusErrorAccountProblem                             ,
    FDSN_StatusErrorAmbiguousGrantByEmailAddress               ,
    FDSN_StatusErrorBadDigest                                  ,
    FDSN_StatusErrorBucketAlreadyExists                        ,
    FDSN_StatusErrorBucketAlreadyOwnedByYou                    ,
    FDSN_StatusErrorBucketNotEmpty                             ,
    FDSN_StatusErrorCredentialsNotSupported                    ,
    FDSN_StatusErrorCrossLocationLoggingProhibited             ,
    FDSN_StatusErrorEntityTooSmall                             ,
    FDSN_StatusErrorEntityTooLarge                             ,

} FDSN_Status;


class ListBucketContents {
public:
   std::string objKey;
   fds_int64_t  lastModified; // Seconds since last modified
   std::string  eTag; // This is similar to MD5
   fds_uint64_t size;
   std::string  ownerId;
   std::string  ownerDisplayName;

   ListBucketContents();
   ~ListBucketContents();
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
 * @param callbackData is the callback data as specified when the request
 *        was issued.
 * @return < 0 to abort the request with the S3StatusAbortedByCallback, which
 *        will be pased to the response complete callback for this request, or
 *        0 to indicate the end of data, or > 0 to identify the number of
 *        bytes that were written into the buffer by this callback
 **/
typedef int (fdsnPutObjectHandler)(void *reqContext, fds_uint64_t bufferSize, char *buffer,
                                      void *callbackData);


/**
 * This callback is made repeatedly, each time
 * providing the next chunk of data read, until the complete object contents
 * have been passed through the callback in this way, or the callback
 * returns an error status.
 *
 * @param bufferSize gives the number of bytes in buffer
 * @param buffer is the data being passed into the callback
 * @param callbackData is the callback data as specified when the request
 *        was issued.
 * @return StatusOK to continue processing the request, anything else to
 *         immediately abort the request with a status which will be
 *         passed to the S3ResponseCompleteCallback for this request.
 *         Typically, this will return either S3StatusOK or
 *         S3StatusAbortedByCallback.
 **/
typedef FDSN_Status (fdsnGetObjectHandler)(void *reqContext, fds_uint64_t bufferSize, const char *buffer,
                                           void *callbackData);
typedef void (fdsnResponseHandler)(FDSN_Status status,
                                          const ErrorDetails *errorDetails,
                                          void *callbackData);
typedef FDSN_Status (fdsnListBucketHandler)(int isTruncated,
                                        const char *nextMarker,
                                        int contentsCount,
                                        const ListBucketContents *contents,
                                        int commonPrefixesCount,
                                        const char **commonPrefixes,
                                        void *callbackData);


// FDS_NativeAPI  object class : One object per client Type so that the semantics of 
// the particular access protocols can be followed in returning the data
// TODO: [Vy] I'd like to make this class as singleton and inherits from
// fds::Module class.
//
class FDS_NativeAPI { 
  public :

  enum FDSN_ClientType {
    FDSN_AWS_S3, 
    FDSN_MSFT_AZURE,
    FDSN_OPEN_STACK_SWIFT,
    FDSN_NATIVE_OBJ_API,
    FDSN_BLOCK_DEVICE
  };
  FDSN_ClientType clientType;

  // TODO: [Vy]: I think this API layer doesn't need to aware of any client's
  // semantics.  Specific semantic is handled by the connector layer.
  //
  FDS_NativeAPI(FDSN_ClientType type);
  ~FDS_NativeAPI(); 

  // Create a bucket
  void CreateBucket(BucketContext *bucket_ctx, CannedAcl  canned_acl, 
                    void *req_ctxt, fdsnResponseHandler responseHandler, void *callback_data);
  // Get the bucket contents  or objets belonging to this bucket
  void GetBucket(BucketContext *bucketContext,
                    std::string prefix, std::string marker,
                    std::string delimiter, fds_uint32_t maxkeys,
                    void *requestContext,
                    fdsnListBucketHandler *handler, void *callbackData);
  void DeleteBucket(BucketContext bucketCtxt,
                    void *requestContext,
                    fdsnResponseHandler *handler, void *callbackData);

  // After this call returns bucketctx, get_cond are no longer valid.
  void GetObject(BucketContext *bucketctxt, 
                 std::string ObjKey, 
                 GetConditions *get_cond, 
                 fds_uint64_t startByte, fds_uint64_t byteCount,
                 char *buffer, fds_uint64_t buflen,
                 void *reqcontext,
                 fdsnGetObjectHandler getObjCallback,
                 void *callbackdata
                 );
  void PutObject(BucketContext *bucket_ctxt, 
                 std::string ObjKey, 
                 PutProperties *putproperties,
                 void *reqContext,
                 char *buffer, fds_uint64_t buflen,
                 fdsnPutObjectHandler putObjHandler, 
                 void *callbackData);
  void DeleteObject(BucketContext *bucket_ctxt, 
                    std::string ObjKey,
                    void *reqcontext, 
                    fdsnResponseHandler responseHandler,
                    void *callbackData);

 private: /* methods */

  /* Checks if bucket is attached to AM, if not, sends test bucket
   * message to OM
   * return error:
   *    ERR_OK if bucket is already attached to AM
   *    ERR_PENDING_RESP if test bucket message was sent to AM and now
   *    we are waiting for response from OM (either we get attach volume 
   *    message or volume does not exist).
   *    Other error codes if error happened.
   */
  Error testBucketInternal(BucketContext *bucket_ctxt);

};
}
#endif 
