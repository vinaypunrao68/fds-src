/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NATIVE_TYPES_H_
#define SOURCE_INCLUDE_NATIVE_TYPES_H_
#include <string>
#include <boost/shared_ptr.hpp>
#include <fdsp/FDSP_types.h>
#include <fds_types.h>
#include <blob/BlobTypes.h>
#include <fds_error.h>
#include <fds_defines.h>
#include <fds_typedefs.h>
#include <util/timeutils.h>
#include "PerfTrace.h"
#include <apis/apis_types.h>
#define FDSN_QOS_PERF_NORMALIZER 20

namespace fds {
    class BucketContext {
  public:
        std::string    hostName;
        std::string   bucketName;
        std::string   accessKeyId;
        std::string   secretAccessKey;

        BucketContext(const std::string& _hostName,
                      const std::string& _bucketName,
                      const std::string& _accessKeyId,
                      const std::string& _secretAccessKey);
        ~BucketContext();
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
                  int _prio);
        ~QosParams();
    };

    class ListBucketContents {
  public:
        std::string objKey;
        fds_int64_t  lastModified;
        std::string  eTag;
        fds_uint64_t size;
        std::string  ownerId;
        std::string  ownerDisplayName;

        ListBucketContents();
        ~ListBucketContents();

        void set(const std::string& _key,
                 fds_uint64_t _modified,
                 const std::string& _eTag,
                 fds_uint64_t _size,
                 const std::string& _ownerId,
                 const std::string& _ownerDisplayName);
    };

    class BucketStatsContent {
  public:
        std::string bucket_name; /* bucket name */
        int priority;            /* bucket's priority */
        double performance;      /* bucket's average iops (timer interval configurable) */
        double sla;              /* bucket's minimum iops */
        double limit;            /* bucket's maximum iops */

        BucketStatsContent();
        ~BucketStatsContent();

        void set(const std::string& _name,
                 int _prio,
                 double _perf,
                 double _sla,
                 double _limit);
    };

    typedef enum {
        AclPermissionRead                    = 0,
        AclPermissionWrite                   = 1,
        AclPermissionReadACP                 = 2,
        AclPermissionWriteACP                = 3,
        AclPermissionFullControl             = 4
    } AclPermission;

    typedef struct ErrorDetails {
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
    typedef enum {
        CannedAclPrivate                  = 0, /* private */
        CannedAclPublicRead               = 1, /* public-read */
        CannedAclPublicReadWrite          = 2, /* public-read-write */
        CannedAclAuthenticatedRead        = 3  /* authenticated-read */
    } CannedAcl;


    typedef enum {
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
    // If ifMatchETag or ifNotMatchETag is non-null then they are valid
    class GetConditions {
        fds_int64_t   ifModifiedSince;
        fds_int64_t   ifNotModifiedSince;
        std::string    ifMatchETag;
        std::string    ifNotMatchETag;
    };

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
                                                void *callbackData,
                                                FDSN_Status status,
                                                ErrorDetails *errDetails);

    typedef void (*fdsnStatBlobHandler)(FDSN_Status status,
                                        const ErrorDetails *errorDetails,
                                        BlobDescriptor blobDesc,
                                        void *callbackData);

    typedef void (*fdsnStartBlobTxHandler)(FDSN_Status status,
                                           const ErrorDetails *errorDetails,
                                           BlobTxId txId,
                                           void *callbackData);

    typedef void (*fdsCommitBlobTxHandler)(FDSN_Status status,
                                           const ErrorDetails *errorDetails,
                                           BlobTxId txId,
                                           void *callbackData);

    typedef void (*fdsnAbortBlobTxHandler)(FDSN_Status status,
                                           const ErrorDetails *errorDetails,
                                           BlobTxId txId,
                                           void *callbackData);

    typedef void (*fdsnResponseHandler)(FDSN_Status status,
                                        const ErrorDetails *errorDetails,
                                        void *callbackData);

    typedef void (*fdsnVolumeContentsHandler)(int isTruncated,
                                              const char *nextMarker,
                                              int contentsCount,
                                              const ListBucketContents *contents,
                                              int commonPrefixesCount,
                                              const char **commonPrefixes,
                                              void *callbackData,
                                              FDSN_Status status);

    typedef void (*fdsnVolumeStatsHandler)(const std::string& timestamp,
                                           int content_count,
                                           const BucketStatsContent* contents,
                                           void *req_context,
                                           void *callbackData,
                                           FDSN_Status status,
                                           ErrorDetails *err_details);

    /**
     * NOTE:
     *   - the member variables should all be return values to be
     *     processed by another consumer.
     *   - all member variables will be owned & destroyed by cb
     *   - these are interfaces :
     *     look at am-engine/handlers for implementation
     */    
    /** New Callback Scheme **/

    /**
     * After filling the callback object , the call() method should be called.
     * Whether the call() method executes immediately / later / in a separate thread
     * is implementation dependent
     */

    struct Callback {
        FDSN_Status status = FDSN_StatusErrorUnknown;
        const ErrorDetails *errorDetails = NULL;
        Error error = ERR_MAX;

        void operator()(FDSN_Status status = FDSN_StatusErrorUnknown);
        void call(FDSN_Status status);
        void call(Error err);
        bool isStatusSet();
        bool isErrorSet();

        virtual void call() = 0;
        virtual ~Callback();
    };
    typedef boost::shared_ptr<Callback> CallbackPtr;


// RAII model.
// NOTE: use this cautiously!!!
// at the end of scope the cb will be called!!
struct ScopedCallBack {
    CallbackPtr cb;
    explicit ScopedCallBack(CallbackPtr cb);
    ~ScopedCallBack();
};

struct StatBlobCallback : virtual Callback {
    typedef boost::shared_ptr<StatBlobCallback> ptr;
    /// The blob descriptor to fill in
    BlobDescriptor      blobDesc;
};

struct StartBlobTxCallback : virtual Callback {
    typedef boost::shared_ptr<StartBlobTxCallback> ptr;
    /// The blob trans ID to fill in
    BlobTxId      blobTxId;
};

struct UpdateBlobCallback : virtual Callback {
    typedef boost::shared_ptr<UpdateBlobCallback> ptr;
};

struct GetObjectCallback : virtual Callback {
    typedef boost::shared_ptr<GetObjectCallback> ptr;
    char *returnBuffer;
    fds_uint32_t returnSize;

    /// Used in getWithMetadata cases
    BlobDescriptor blobDesc;
};

struct CommitBlobTxCallback : virtual Callback {
    typedef boost::shared_ptr<CommitBlobTxCallback> ptr;
    /// The blob trans ID to fill in
    BlobTxId      blobTxId;
};

struct AbortBlobTxCallback : virtual Callback {
    typedef boost::shared_ptr<AbortBlobTxCallback> ptr;
    /// The blob trans ID to fill in
    BlobTxId      blobTxId;
};

struct GetVolumeMetaDataCallback : virtual Callback {
    TYPE_SHAREDPTR(GetVolumeMetaDataCallback);
    fpi::FDSP_VolumeMetaData volumeMetaData;
};

struct GetBucketCallback : virtual Callback {
    TYPE_SHAREDPTR(GetBucketCallback);
    int isTruncated = 0;
    const char *nextMarker = NULL;
    int contentsCount = 0;
    const ListBucketContents *contents = NULL;
    int commonPrefixesCount = 0;
    const char **commonPrefixes = NULL;

    std::vector<apis::BlobDescriptor> vecBlobs;
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_NATIVE_TYPES_H_
