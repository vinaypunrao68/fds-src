/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_FDS_ERROR_H_
#define SOURCE_INCLUDE_FDS_ERROR_H_

#include <sstream>
#include <string>
#include <shared/fds_types.h>
#include <fdsp/FDSP_types.h>

namespace fds {

// TODO(Rao): There is already a mismatch between enums and strings.
// Ideally this errstrs isn't needed.  We need an error string catalog
// that we lookup.  We need to remove this array.
extern const char* fds_errstrs[];

/* DO NOT change the order */
typedef enum {
    ERR_OK                   = 0,
    ERR_DUPLICATE            = 1,
    ERR_HASH_COLLISION       = 2,
    ERR_DISK_WRITE_FAILED    = 3,
    ERR_DISK_READ_FAILED     = 4,
    ERR_CAT_QUERY_FAILED     = 5,
    ERR_CAT_ENTRY_NOT_FOUND  = 6,
    ERR_INVALID_ARG          = 7,
    ERR_PENDING_RESP         = 8,
    ERR_NOT_FOUND            = 9,
    ERR_VOL_ADMISSION_FAILED = 10,
    ERR_GET_DLT_FAILED       = 11,
    ERR_GET_DMT_FAILED       = 12,
    ERR_NOT_IMPLEMENTED      = 13,
    ERR_OUT_OF_MEMORY        = 14,
    ERR_DUPLICATE_UUID       = 15,
    ERR_TRANS_JOURNAL_OUT_OF_IDS = 16,
    ERR_TRANS_JOURNAL_REQUEST_QUEUED = 17,
    ERR_NODE_NOT_ACTIVE      = 18,
    ERR_VOL_NOT_EMPTY        = 19,
    ERR_BLOB_NOT_FOUND       = 20,
    ERR_NOT_READY            = 21,
    ERR_INVALID_DLT          = 22,
    ERR_PERSIST_STATE_MISMATCH = 23,
    ERR_EXCEED_MIN_IOPS      = 24,
    ERR_UNAUTH_ACCESS        = 25,
    ERR_DISK_FILE_UNLINK_FAILED = 26,
    ERR_SERIALIZE_FAILED     = 27,
    ERR_NO_BYTES_WRITTEN     = 28,
    ERR_NO_BYTES_READ        = 29,
    ERR_VOL_NOT_FOUND        = 30,
    ERR_VOL_DUPLICATE        = 31,
    ERR_NETWORK_CORRUPT      = 32,
    ERR_ONDISK_DATA_CORRUPT  = 33,
    ERR_INVALID_DMT          = 34,
    ERR_CATSYNC_NOT_PROGRESS = 35,
    ERR_DM_RSYNC_FAILED       = 36,
    ERR_DMT_FORWARD          = 37,

    ERR_DM_TX_EXISTS          = 38,
    ERR_DM_TX_NOT_STARTED     = 39,
    ERR_DM_TX_COMMITTED       = 40,
    ERR_DM_OFFSET_OUT_RANGE   = 41,
    ERR_DM_TX_ACTIVE          = 42,
    ERR_DM_MAX_CL_ENTRIES     = 43,
    ERR_DM_VOL_MARKED_DELETED = 44,
    ERR_DM_OP_NOT_ALLOWED     = 45,
    ERR_DM_SNAPSHOT_FAILED    = 46,
    ERR_INVALID_VOL_ID        = 47,
    ERR_DM_JOURNAL_TIME       = 48,
    ERR_DM_REPLAY_JOURNAL     = 49,
    ERR_DM_FORWARD_FAILED     = 50,
    ERR_SHUTTING_DOWN         = 51,
    ERR_DM_MIGRATION_ABORTED  = 52,
    ERR_DM_VOL_NOT_ACTIVATED  = 53,
    ERR_DM_INVALID_TX_ID      = 54,
    ERR_DLT_IO_PENDING        = 55,

    ERR_ARCHIVE_SNAP_TAR_FAILED    = 56,
    ERR_ARCHIVE_PUT_FAILED         = 57,
    ERR_ARCHIVE_GET_FAILED         = 58,
    ERR_VOLUME_ACCESS_DENIED       = 59,
    ERR_TOKEN_NOT_READY            = 60,
    ERR_DM_INVALID_REGEX           = 61,
    ERR_DM_NOT_PRIMARY             = 62,
    ERR_DM_NOT_TOP_PRIMARY         = 63,
    ERR_SERVICE_CAPACITY_FULL      = 64,
    ERR_SERVICE_CAPACITY_DANGEROUS = 65,

    /* I/O error range */
    ERR_IO_DLT_MISMATCH      = 100,
    ERR_IO_DMT_MISMATCH      = 101,

    /* Metadata error range */
    ERR_BLOB_OFFSET_INVALID  = 500,

    /* DM Migration error range [1000-1500) */
    ERR_MIGRATION_DUPLICATE_REQUEST = 1000,
    ERR_DM_CAT_MIGRATION_ABORTED,
    ERR_DM_CAT_MIGRATION_TIMEOUT,
    ERR_DM_CAT_MIGRATION_DIFF_FAILED,
    ERR_DM_CAT_MIGRATION_APPLY_BLOB_OFFSET_FAILURE,
    ERR_DM_CAT_MIGRATION_APPLY_BLOB_DESC_FAILURE,
    ERR_DM_CAT_MIGRATION_DEST_MSG_CORRUPT,
    ERR_DM_CAT_MIGRATION_SOURCE_MSG_CORRUPT,

    /* FdsActor err range [1500-2000) */
    ERR_FAR_INVALID_REQUEST = 1500,
    /* FDS actor is shutdown */
    ERR_FAR_SHUTDOWN,

    /* Storage manager error range [2000-2999) */
    ERR_SM_NOT_IN_SYNC_MODE = 2000,
    ERR_SM_TOKENSTATEDB_KEY_NOT_FOUND,
    ERR_SM_TOKENSTATEDB_DUPLICATE_KEY,
    ERR_SM_OBJECT_DATA_MISSING,
    ERR_SM_ZERO_REFCNT_OBJECT,
    ERR_SM_OLT_DISKS_INCONSISTENT,
    ERR_SM_SUPERBLOCK_MISSING_FILE,
    ERR_SM_SUPERBLOCK_CHECKSUM_FAIL,
    ERR_SM_SUPERBLOCK_DATA_CORRUPT,
    ERR_SM_SUPERBLOCK_READ_FAIL,
    ERR_SM_SUPERBLOCK_WRITE_FAIL,
    ERR_SM_SUPERBLOCK_NO_RECONCILE,
    ERR_SM_SUPERBLOCK_INCONSISTENT,
    ERR_SM_NOERR_PRISTINE_STATE,
    ERR_SM_GC_ENABLED,
    ERR_SM_GC_TEMP_DISABLED,
    ERR_SM_AUTO_GC_FAILED,
    ERR_SM_DUP_OBJECT_CORRUPT,
    ERR_SM_SHUTTING_DOWN,
    ERR_SM_TOK_MIGRATION_DESTINATION_MSG_CORRUPT,
    ERR_SM_TOK_MIGRATION_SOURCE_MSG_CORRUPT,
    ERR_SM_TOK_MIGRATION_NO_DATA_RECVD,
    ERR_SM_TOK_MIGRATION_DATA_MISMATCH,
    ERR_SM_TOK_MIGRATION_METADATA_MISMATCH,
    ERR_SM_TOK_MIGRATION_ABORTED,
    ERR_SM_TOK_MIGRATION_TIMEOUT,
    ERR_SM_TOK_MIGRATION_INPROGRESS,
    ERR_SM_TOK_MIGRATION_SRC_SVC_REQUEST,
    ERR_SM_TIER_WRITEBACK_NOT_DONE,
    ERR_SM_TIER_HYBRIDMOVE_ON_FLASH_VOLUME,
    ERR_SM_EXCEEDED_DISK_CAPACITY,
    ERR_SM_NOERR_GAINED_SM_TOKENS,
    ERR_SM_NOERR_LOST_SM_TOKENS,
    ERR_SM_NOERR_NEED_RESYNC,
    ERR_SM_NOERR_NOT_IN_DLT,
    ERR_SM_RESYNC_SOURCE_DECLINE,
    ERR_SM_NOT_READY_AS_MIGR_SRC,
    ERR_SM_NO_DISK,

    /* Network errors */
    ERR_NETWORK_TRANSPORT = 3000,
    /* Endpoint doesn't exist */
    ERR_EP_NON_EXISTANT,

    /* SVC Request realted */
    ERR_SVC_REQUEST_FAILED = 4000,
    ERR_SVC_REQUEST_INVOCATION,
    ERR_SVC_REQUEST_USER_INTERRUPTED,
    ERR_SVC_REQUEST_TIMEOUT,
    ERR_SVC_SERVER_PORT_ALREADY_INUSE,
    ERR_SVC_SERVER_CRASH,

    /* FDSN status errors */
    // TODO(Rao): Change FDSN_Status prefix to ERR_ prefix
    FDSN_StatusCreated = 5000                                  ,
    /* do denote uninitialized value*/
    FDSN_StatusNOTSET                                          ,

    /**
     * Errors that prevent the S3 request from being issued or response from
     * being read
     **/
    FDSN_StatusInternalError                                   ,
    FDSN_StatusOutOfMemory                                     ,
    FDSN_StatusInterrupted                                     ,
    FDSN_StatusTxnInProgress                                   ,
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
    FDSN_StatusRequestTimedOut                                 ,
    FDSN_StatusEntityEmpty                                     ,
    FDSN_StatusEntityDoesNotExist                              ,
    /**
     * Errors from the S3 service
     **/
    FDSN_StatusErrorAccessDenied                               ,
    FDSN_StatusErrorAccountProblem                             ,
    FDSN_StatusErrorAmbiguousGrantByEmailAddress               ,
    FDSN_StatusErrorBadDigest                                  ,
    FDSN_StatusErrorBucketAlreadyExists                        ,
    FDSN_StatusErrorBucketNotExists                            ,
    FDSN_StatusErrorBucketAlreadyOwnedByYou                    ,
    FDSN_StatusErrorBucketNotEmpty                             ,
    FDSN_StatusErrorCredentialsNotSupported                    ,
    FDSN_StatusErrorCrossLocationLoggingProhibited             ,
    FDSN_StatusErrorEntityTooSmall                             ,
    FDSN_StatusErrorEntityTooLarge                             ,

    FDSN_StatusErrorMissingContentLength                       ,

    /*
     * Errors for Platform things
     */
    PLATFORM_ERROR_UNEXPECTED_CHILD_DEATH               = 60000,

    /* keep this as the last*/
    FDSN_StatusErrorUnknown                                     ,
    /* Generic catch all error.  DON'T USE IT, unless you don't have an option */
    ERR_INVALID,

    ERR_MAX
} fds_errno_t;

typedef fds_errno_t FDSN_Status;

class Error {
    fds_errno_t _errno;

  public:
    Error() : _errno(ERR_OK) {}
    Error(fds_errno_t errno_arg) : _errno(errno_arg) {} //NOLINT
    Error(fds_uint32_t errno_fdsp) : _errno(static_cast<fds_errno_t>(errno_fdsp)) {} //NOLINT
    Error(const Error& err) : _errno(err._errno) {}

    bool OK() const
    { return _errno == ERR_OK; }

    bool ok() const
    { return OK(); }

    fds_errno_t GetErrno() const
    { return _errno; }

    std::string GetErrstr() const;

    FDS_ProtocolInterface::FDSP_ErrType getFdspErr() const;

    Error& operator= (const Error& rhs)
    { if (this != &rhs) _errno = rhs._errno; return *this; }

    Error& operator= (const fds_errno_t& rhs)
    { _errno = rhs; return *this; }

    explicit operator bool() const
    { return OK(); }

    friend bool operator== (const Error& lhs, const Error& rhs);
    friend bool operator!= (const Error& lhs, const Error& rhs);
};

struct ErrorHash {
    size_t operator()(const Error& e) const
    { return e.GetErrno(); }
};

inline std::string Error::GetErrstr() const {
    if (_errno <= ERR_INVALID_DMT) return fds_errstrs[_errno];
    return std::string("Error no: ") + std::to_string(_errno);
}

inline FDS_ProtocolInterface::FDSP_ErrType Error::getFdspErr() const {
    return (OK() ? FDS_ProtocolInterface::FDSP_ERR_OKOK :
            FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE);
}

std::ostream& operator<<(std::ostream& out, const Error& err);

std::ostream& operator<<(std::ostream& os, FDSN_Status status);

FDSN_Status getStatusFromError(const Error& error);
std::string toString(FDSN_Status status);

inline bool operator== (const Error& lhs, const Error& rhs) {
    return (lhs._errno == rhs._errno);
}

inline bool operator!= (const Error& lhs, const Error& rhs) {
    return !(lhs == rhs);
}

struct Exception : std::exception {
    explicit Exception(const Error& e, const std::string& message="") throw();
    explicit Exception(const std::string& message) throw();
    Exception(const Exception&) throw();
    Exception& operator= (const Exception&) throw();
    const Error& getError() const;
    virtual ~Exception() throw();
    virtual const char* what() const throw();

  protected:
    Error err;
    std::string message;
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_ERROR_H_
