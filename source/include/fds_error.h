/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_FDS_ERROR_H_
#define SOURCE_INCLUDE_FDS_ERROR_H_

#include <sstream>
#include <string>
#include <map>
#include <shared/fds_types.h>
#include <fdsp/FDSP_types.h>
#include <util/enum_util.h>

namespace fds {
    
#define FDS_ERRORNO_ENUM_VALUES(ADD) \
    ADD(ERR_OK,= 0,"ALL OK"), \
    ADD(ERR_DUPLICATE,= 1,"Data is a duplicate"), \
    ADD(ERR_HASH_COLLISION,= 2,"Hash digest collision"), \
    ADD(ERR_DISK_WRITE_FAILED,= 3,"Unable to write data to disk"), \
    ADD(ERR_DISK_READ_FAILED,= 4,"Unable to read data from disk"), \
    ADD(ERR_CAT_QUERY_FAILED,= 5,"Unable to query into catalog"), \
    ADD(ERR_CAT_ENTRY_NOT_FOUND,= 6,"Catalog entry not found"), \
    ADD(ERR_INVALID_ARG,= 7,"Invalid argument or parameter"), \
    ADD(ERR_PENDING_RESP,= 8,"Response is pending"), \
    ADD(ERR_NOT_FOUND,= 9,"Not found"), \
    ADD(ERR_VOL_ADMISSION_FAILED,= 10,"Admission control for a volume failed"), \
    ADD(ERR_GET_DLT_FAILED,= 11,"Get DLT failed"), \
    ADD(ERR_GET_DMT_FAILED,= 12,"Get DMT failed"), \
    ADD(ERR_NOT_IMPLEMENTED,= 13,"Feature not implemented"), \
    ADD(ERR_OUT_OF_MEMORY,= 14,"Out of memory"), \
    ADD(ERR_DUPLICATE_UUID,= 15,"Node name hashed to duplicate UUID"), \
    ADD(ERR_TRANS_JOURNAL_OUT_OF_IDS,= 16,"Ran out of transaction ids"), \
    ADD(ERR_TRANS_JOURNAL_REQUEST_QUEUED,= 17,"Transaction request is queued"), \
    ADD(ERR_NODE_NOT_ACTIVE,= 18,"Node is not in active state"), \
    ADD(ERR_VOL_NOT_EMPTY,= 19,"Volume is not empty"), \
    ADD(ERR_BLOB_NOT_FOUND,= 20,"Blob does not exist"), \
    ADD(ERR_NOT_READY,= 21,"Not ready, re-try later"), \
    ADD(ERR_INVALID_DLT,= 22,"Invalid DLT"), \
    ADD(ERR_PERSIST_STATE_MISMATCH,= 23,"Mismatch in persistent state"), \
    ADD(ERR_EXCEED_MIN_IOPS,= 24,"Operation not permitted: would exceed total min iops"), \
    ADD(ERR_UNAUTH_ACCESS,= 25,"Unauthorized access"), \
    ADD(ERR_DISK_FILE_UNLINK_FAILED,= 26,"Failed to unlink file"), \
    ADD(ERR_SERIALIZE_FAILED,= 27,"Failed to serialize data"), \
    ADD(ERR_NO_BYTES_WRITTEN,= 28,"No bytes were written"), \
    ADD(ERR_NO_BYTES_READ,= 29,"No bytes were read"), \
    ADD(ERR_VOL_NOT_FOUND,= 30,"Volume not found"), \
    ADD(ERR_VOL_DUPLICATE,= 31,"Duplicate volume"), \
    ADD(ERR_NETWORK_CORRUPT,= 32,"Object Buffer corrupted network"), \
    ADD(ERR_ONDISK_DATA_CORRUPT,= 33,"OnDisk Object Buffer corrupt"), \
    ADD(ERR_INVALID_DMT,= 34,"Invalid DMT"), \
    ADD(ERR_CATSYNC_NOT_PROGRESS,= 35,"Catalog sync is not in progress"), \
    ADD(ERR_DM_RSYNC_FAILED,= 36,"Rsync failed to sync volume catalog in DM"), \
    ADD(ERR_DMT_FORWARD,= 37,"Forward DM request"), \
                                    \
    ADD(ERR_DM_TX_EXISTS,= 38,"Transacton already exists in DM commit log"), \
    ADD(ERR_DM_TX_NOT_STARTED,= 39,"Transaction not started"), \
    ADD(ERR_DM_TX_COMMITTED,= 40,"Transaction already committed"), \
    ADD(ERR_DM_OFFSET_OUT_RANGE,= 41,"Offset in blob out of range"), \
    ADD(ERR_DM_TX_ACTIVE,= 42,"Transaction active"), \
    ADD(ERR_DM_MAX_CL_ENTRIES,= 43,"Reached max commit log entries"), \
    ADD(ERR_DM_VOL_MARKED_DELETED,= 44,"Volume marked as deleted"), \
    ADD(ERR_DM_OP_NOT_ALLOWED,= 45,"Operation not allowed"), \
    ADD(ERR_DM_SNAPSHOT_FAILED,= 46,"Snapshot failed"), \
    ADD(ERR_INVALID_VOL_ID,= 47,"Invalid volume ID"), \
    ADD(ERR_DM_JOURNAL_TIME,= 48,"Journal time not available"), \
    ADD(ERR_DM_REPLAY_JOURNAL,= 49,"Unable to replay journal for provided interval"), \
    ADD(ERR_DM_FORWARD_FAILED,= 50,"DM forwarding failed"), \
    ADD(ERR_SHUTTING_DOWN,= 51,"Service is shutting down"), \
    ADD(ERR_DM_MIGRATION_ABORTED,= 52,"DM migration aborted"), \
    ADD(ERR_DM_VOL_NOT_ACTIVATED,= 53,"DM volume not activated"), \
    ADD(ERR_DM_INVALID_TX_ID,= 54,"Invalid transaction ID"), \
    ADD(ERR_IO_PENDING,= 55,"There are inflight IOs for the current Table version"), \
                                    \
    ADD(ERR_ARCHIVE_SNAP_TAR_FAILED,= 56,"Failed to tar snap"), \
    ADD(ERR_ARCHIVE_PUT_FAILED,= 57,"Archive put failed"), \
    ADD(ERR_ARCHIVE_GET_FAILED,= 58,"Archive get failed"), \
    ADD(ERR_VOLUME_ACCESS_DENIED,= 59,"Volume access denied"), \
    ADD(ERR_TOKEN_NOT_READY,= 60,"Token not ready"), \
    ADD(ERR_DM_INVALID_REGEX,= 61,"DM invalid regex"), \
    ADD(ERR_DM_NOT_PRIMARY,= 62,"Not one of the primaries"), \
    ADD(ERR_DM_NOT_TOP_PRIMARY,= 63,"Not the first primary"), \
    ADD(ERR_SERVICE_CAPACITY_FULL,= 64,"Service ran out of disk capacity"), \
    ADD(ERR_SERVICE_CAPACITY_DANGEROUS,= 65,"Service is at dangerous used disk capacity level"), \
    ADD(ERR_DM_UNRECOGNIZED_PATTERN_SEMANTICS,= 66,"Pattern semantics not recognized"), \
    ADD(ERR_TIMER_TASK_NOT_SCHEDULED,= 67,"Timer task was not scheduled"), \
    ADD(ERR_FEATURE_DISABLED,= 68,"Feature disabled"), \
    ADD(ERR_DM_INTERRUPTED,=69,"Process interrupted"), \
    \
    /* I/O error range */            \
    ADD(ERR_IO_DLT_MISMATCH,= 100," "), \
    ADD(ERR_IO_DMT_MISMATCH,= 101," "), \
    \
    /* Metadata error range */          \
    ADD(ERR_BLOB_OFFSET_INVALID,= 500," "), \
    ADD(ERR_BLOB_SEQUENCE_ID_REGRESSION, ,"  "), \
    \
    /* DM Migration error range [1000-1500"), */  \
    ADD(ERR_MIGRATION_DUPLICATE_REQUEST,= 1000," "), \
    ADD(ERR_DM_CAT_MIGRATION_ABORTED, ,"  "), \
    ADD(ERR_DM_CAT_MIGRATION_TIMEOUT, ,"  "), \
    ADD(ERR_DM_CAT_MIGRATION_DIFF_FAILED, ,"  "), \
    ADD(ERR_DM_CAT_MIGRATION_APPLY_BLOB_OFFSET_FAILURE, ,"  "), \
    ADD(ERR_DM_CAT_MIGRATION_APPLY_BLOB_DESC_FAILURE, ,"  "), \
    ADD(ERR_DM_CAT_MIGRATION_DEST_MSG_CORRUPT, ,"  "), \
    ADD(ERR_DM_CAT_MIGRATION_SOURCE_MSG_CORRUPT, ,"  "), \
    \
    /* FdsActor err range [1500-2000"), */        \
    ADD(ERR_FAR_INVALID_REQUEST,= 1500," "), \
    /* FDS actor is shutdown */     \
    ADD(ERR_FAR_SHUTDOWN, ,"  "), \
    \
    /* Storage manager error range [2000-2999"), */   \
    ADD(ERR_SM_NOT_IN_SYNC_MODE,= 2000," "), \
    ADD(ERR_SM_TOKENSTATEDB_KEY_NOT_FOUND, ,"  "), \
    ADD(ERR_SM_TOKENSTATEDB_DUPLICATE_KEY, ,"  "), \
    ADD(ERR_SM_OBJECT_DATA_MISSING, ,"  "), \
    ADD(ERR_SM_ZERO_REFCNT_OBJECT, ,"  "), \
    ADD(ERR_SM_OLT_DISKS_INCONSISTENT, ,"  "), \
    ADD(ERR_SM_SUPERBLOCK_MISSING_FILE, ,"  "), \
    ADD(ERR_SM_SUPERBLOCK_CHECKSUM_FAIL, ,"  "), \
    ADD(ERR_SM_SUPERBLOCK_DATA_CORRUPT, ,"  "), \
    ADD(ERR_SM_SUPERBLOCK_READ_FAIL, ,"  "), \
    ADD(ERR_SM_SUPERBLOCK_WRITE_FAIL, ,"  "), \
    ADD(ERR_SM_SUPERBLOCK_NO_RECONCILE, ,"  "), \
    ADD(ERR_SM_SUPERBLOCK_INCONSISTENT, ,"  "), \
    ADD(ERR_SM_NOERR_PRISTINE_STATE, ,"  "), \
    ADD(ERR_SM_GC_ENABLED, ,"  "), \
    ADD(ERR_SM_GC_TEMP_DISABLED, ,"  "), \
    ADD(ERR_SM_AUTO_GC_FAILED, ,"  "), \
    ADD(ERR_SM_DUP_OBJECT_CORRUPT, ,"  "), \
    ADD(ERR_SM_SHUTTING_DOWN, ,"  "), \
    ADD(ERR_SM_TOK_MIGRATION_DESTINATION_MSG_CORRUPT, ,"  "), \
    ADD(ERR_SM_TOK_MIGRATION_SOURCE_MSG_CORRUPT, ,"  "), \
    ADD(ERR_SM_TOK_MIGRATION_NO_DATA_RECVD, ,"  "), \
    ADD(ERR_SM_TOK_MIGRATION_DATA_MISMATCH, ,"  "), \
    ADD(ERR_SM_TOK_MIGRATION_METADATA_MISMATCH, ,"  "), \
    ADD(ERR_SM_TOK_MIGRATION_ABORTED, ,"  "), \
    ADD(ERR_SM_TOK_MIGRATION_TIMEOUT, ,"  "), \
    ADD(ERR_SM_TOK_MIGRATION_INPROGRESS, ,"  "), \
    ADD(ERR_SM_TOK_MIGRATION_SRC_SVC_REQUEST, ,"  "), \
    ADD(ERR_SM_TIER_WRITEBACK_NOT_DONE, ,"  "), \
    ADD(ERR_SM_TIER_HYBRIDMOVE_ON_FLASH_VOLUME, ,"  "), \
    ADD(ERR_SM_EXCEEDED_DISK_CAPACITY, ,"  "), \
    ADD(ERR_SM_NOERR_GAINED_SM_TOKENS, ,"  "), \
    ADD(ERR_SM_NOERR_LOST_SM_TOKENS, ,"  "), \
    ADD(ERR_SM_NOERR_NEED_RESYNC, ,"  "), \
    ADD(ERR_SM_NOERR_NOT_IN_DLT, ,"  "), \
    ADD(ERR_SM_RESYNC_SOURCE_DECLINE, ,"  "), \
    ADD(ERR_SM_NOT_READY_AS_MIGR_SRC, ,"  "), \
    ADD(ERR_SM_NO_DISK, ,"  "), \
    \
    /* Network errors */    \
    ADD(ERR_NETWORK_TRANSPORT,= 3000," "), \
    /* Endpoint doesn't exist */    \
    ADD(ERR_EP_NON_EXISTANT, ,"  "),        \
    \
    /* File Errors */ \
    ADD(ERR_FILE_DOES_NOT_EXIST,= 3100 ,"  "),      \
    ADD(ERR_CHECKSUM_MISMATCH, ,"  "),      \
    \
    /* SVC Request realted */   \
    ADD(ERR_SVC_REQUEST_FAILED,= 4000," "), \
    ADD(ERR_SVC_REQUEST_INVOCATION, ,"  "), \
    ADD(ERR_SVC_REQUEST_USER_INTERRUPTED, ,"  "), \
    ADD(ERR_SVC_REQUEST_TIMEOUT, ,"  "), \
    ADD(ERR_SVC_SERVER_PORT_ALREADY_INUSE, ,"  "), \
    ADD(ERR_SVC_SERVER_CRASH, ,"  "), \
    \
    /* FDSN status errors */    \
    /* TODO(Rao"),: Change FDSN_Status prefix to ERR_ prefix */  \
    ADD(FDSN_StatusCreated,= 5000," "), \
    /* do denote uninitialized value*/  \
    ADD(FDSN_StatusNOTSET, ,"  "), \
    \
    /* Errors that prevent the S3 request from being issued or response from being read */ \
    ADD(FDSN_StatusInternalError, ,"  "), \
    ADD(FDSN_StatusOutOfMemory, ,"  "), \
    ADD(FDSN_StatusInterrupted, ,"  "), \
    ADD(FDSN_StatusTxnInProgress, ,"  "), \
    ADD(FDSN_StatusInvalidBucketNameTooLong, ,"  "), \
    ADD(FDSN_StatusInvalidBucketNameFirstCharacter, ,"  "), \
    ADD(FDSN_StatusInvalidBucketNameCharacter, ,"  "), \
    ADD(FDSN_StatusInvalidBucketNameCharacterSequence, ,"  "), \
    ADD(FDSN_StatusInvalidBucketNameTooShort, ,"  "), \
    ADD(FDSN_StatusInvalidBucketNameDotQuadNotation, ,"  "), \
    ADD(FDSN_StatusQueryParamsTooLong, ,"  "), \
    ADD(FDSN_StatusFailedToInitializeRequest, ,"  "), \
    ADD(FDSN_StatusMetaDataHeadersTooLong, ,"  "), \
    ADD(FDSN_StatusBadMetaData, ,"  "), \
    ADD(FDSN_StatusBadContentType, ,"  "), \
    \
    ADD(FDSN_StatusContentTypeTooLong, ,"  "), \
    ADD(FDSN_StatusBadMD5, ,"  "), \
    ADD(FDSN_StatusMD5TooLong, ,"  "), \
    ADD(FDSN_StatusBadCacheControl, ,"  "), \
    ADD(FDSN_StatusCacheControlTooLong, ,"  "), \
    ADD(FDSN_StatusBadContentDispositionFilename, ,"  "), \
    ADD(FDSN_StatusContentDispositionFilenameTooLong, ,"  "), \
    ADD(FDSN_StatusBadContentEncoding, ,"  "), \
    ADD(FDSN_StatusContentEncodingTooLong, ,"  "), \
    ADD(FDSN_StatusBadIfMatchETag, ,"  "), \
    ADD(FDSN_StatusIfMatchETagTooLong, ,"  "), \
    ADD(FDSN_StatusBadIfNotMatchETag, ,"  "), \
    ADD(FDSN_StatusIfNotMatchETagTooLong, ,"  "), \
    ADD(FDSN_StatusHeadersTooLong, ,"  "), \
    ADD(FDSN_StatusKeyTooLong, ,"  "), \
    ADD(FDSN_StatusUriTooLong, ,"  "), \
    ADD(FDSN_StatusXmlParseFailure, ,"  "), \
    ADD(FDSN_StatusEmailAddressTooLong, ,"  "), \
    ADD(FDSN_StatusUserIdTooLong, ,"  "), \
    ADD(FDSN_StatusUserDisplayNameTooLong, ,"  "), \
    ADD(FDSN_StatusGroupUriTooLong, ,"  "), \
    ADD(FDSN_StatusPermissionTooLong, ,"  "), \
    ADD(FDSN_StatusTargetBucketTooLong, ,"  "), \
    ADD(FDSN_StatusTargetPrefixTooLong, ,"  "), \
    ADD(FDSN_StatusTooManyGrants, ,"  "), \
    ADD(FDSN_StatusBadGrantee, ,"  "), \
    ADD(FDSN_StatusBadPermission, ,"  "), \
    ADD(FDSN_StatusXmlDocumentTooLarge, ,"  "), \
    ADD(FDSN_StatusNameLookupError, ,"  "), \
    ADD(FDSN_StatusFailedToConnect, ,"  "), \
    ADD(FDSN_StatusServerFailedVerification, ,"  "), \
    ADD(FDSN_StatusConnectionFailed, ,"  "), \
    ADD(FDSN_StatusAbortedByCallback, ,"  "), \
    ADD(FDSN_StatusRequestTimedOut, ,"  "), \
    ADD(FDSN_StatusEntityEmpty, ,"  "), \
    ADD(FDSN_StatusEntityDoesNotExist, ,"  "), \
    /* Errors from the S3 service */    \
    ADD(FDSN_StatusErrorAccessDenied, ,"  "), \
    ADD(FDSN_StatusErrorAccountProblem, ,"  "), \
    ADD(FDSN_StatusErrorAmbiguousGrantByEmailAddress, ,"  "), \
    ADD(FDSN_StatusErrorBadDigest, ,"  "), \
    ADD(FDSN_StatusErrorBucketAlreadyExists, ,"  "), \
    ADD(FDSN_StatusErrorBucketNotExists, ,"  "), \
    ADD(FDSN_StatusErrorBucketAlreadyOwnedByYou, ,"  "), \
    ADD(FDSN_StatusErrorBucketNotEmpty, ,"  "), \
    ADD(FDSN_StatusErrorCredentialsNotSupported, ,"  "), \
    ADD(FDSN_StatusErrorCrossLocationLoggingProhibited, ,"  "), \
    ADD(FDSN_StatusErrorEntityTooSmall, ,"  "), \
    ADD(FDSN_StatusErrorEntityTooLarge, ,"  "), \
    \
    ADD(FDSN_StatusErrorMissingContentLength, ,"  "), \
    \
    /* Errors for Platform things */ \
    ADD(PLATFORM_ERROR_UNEXPECTED_CHILD_DEATH,= 60000," "), \
    \
    /* keep this as the last*/  \
    ADD(FDSN_StatusErrorUnknown, ,"  "), \
    /* Generic catch all error.  DON'T USE IT, unless you don't have an option */   \
    ADD(ERR_INVALID, ,"  "), \
    \
    ADD(ERR_MAX, ,"  ")
    
    /* DO NOT change the order */
typedef enum {
    FDS_ERRORNO_ENUM_VALUES(ADD_TO_ENUM)
} fds_errno_t;

typedef fds_errno_t FDSN_Status;

extern std::map<int, enumDescription> fds_errno_desc;

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
    std::string GetErrName() const;

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
    auto enumDesc = fds_errno_desc.find(_errno);
    if (enumDesc == fds_errno_desc.end()) {
        return std::string("Error no: ") + std::to_string(_errno);
    } else {
        return enumDesc->second.second;
    }
}

inline std::string Error::GetErrName() const {
    auto enumDesc = fds_errno_desc.find(_errno);
    if (enumDesc == fds_errno_desc.end()) {
        return std::string("Error no: ") + std::to_string(_errno);
    } else {
        return enumDesc->second.first;
    }
}

inline FDS_ProtocolInterface::FDSP_ErrType Error::getFdspErr() const {
    return (OK() ? FDS_ProtocolInterface::FDSP_ERR_OKOK :
            FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE);
}

std::ostream& operator<<(std::ostream& out, const Error& err);

std::ostream& operator<<(std::ostream& os, FDSN_Status status);

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
