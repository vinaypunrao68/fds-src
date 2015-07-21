/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <fds_error.h>
#include <string>
#include <fds_defines.h>
namespace fds {

const char* fds_errstrs[] = {
    "ALL OK",
    "Data is a duplicate",
    "Hash digest collision",
    "Unable to write data to disk",
    "Unable to read data from disk",
    "Unable to query into catalog",
    "Catalog entry not found",
    "Invalid argument or parameter",
    "Response is pending",
    "Not found",
    "Admission control for a volume failed",
    "Get DLT failed",
    "Get DMT failed",
    "Feature not implemented",
    "Out of memory",
    "Node name hashed to duplicate UUID",
    "Ran out of transaction ids",
    "Transaction request is queued",
    "Node is not in active state",
    "Volume is not empty",
    "Blob does not exist",
    "Not ready, re-try later",
    "Invalid DLT",
    "Mismatch in persistent state",
    "Operation not permitted: would exceed total min iops",
    "Unauthorized access",
    "Failed to unlink file",
    "Failed to serialize data",
    "No bytes were written",
    "No bytes were read",
    "Volume not found",
    "Duplicate volume",
    "Object Buffer corrupted network",
    "OnDisk Object Buffer corrupt",
    "Invalid DMT",
    "Catalog sync is not in progress",
    "Rsync failed to sync volume catalog in DM",
    "Forward DM request",
    "Transacton already exists in DM commit log",
    "Transaction not started",
    "Transaction already committed",
    "Offset in blob out of range",
    "Transaction active",
    "Reached max commit log entries",
    "Volume marked as deleted",
    "Operation not allowed",
    "Snapshot failed",
    "Invalid volume ID",
    "Journal time not available",
    "Unable to replay journal for provided interval",
    "DM forwarding failed",
    "Service is shutting down",
    "DM migration aborted",
    "DM volume not activated",
    "Invalid transaction ID",
    "There are inflight IOs for the current DLT version",
    "Failed to tar snap",
    "Archive put failed",
    "Archive get failed",
    "Volume access denied",
    "Token not ready",
    "DM invalid regex",
    "Not one of the primaries",
    "Not the first primary",
    "Service ran out of disk capacity",
    "Service is at dangerous used disk capacity level",
    // Good up to here. 01/28/2015
    "SM object metadata not found",
    "error during network communication"
    "Unauthorized access to the object/bucket "
};

std::ostream& operator<<(std::ostream& out, const Error& err) {
    return out << "Error " << err.GetErrno() << ": " << err.GetErrstr();
}

std::ostream& operator<<(std::ostream& os, FDSN_Status status) {
    switch (status) {
        ENUMCASEOS(FDSN_StatusNOTSET                             , os);
        ENUMCASEOS(ERR_OK                                        , os);
        ENUMCASEOS(FDSN_StatusCreated                            , os);
        ENUMCASEOS(FDSN_StatusInternalError                      , os);
        ENUMCASEOS(FDSN_StatusOutOfMemory                        , os);
        ENUMCASEOS(FDSN_StatusInterrupted                        , os);
        ENUMCASEOS(FDSN_StatusTxnInProgress                      , os);
        ENUMCASEOS(FDSN_StatusInvalidBucketNameTooLong           , os);
        ENUMCASEOS(FDSN_StatusInvalidBucketNameFirstCharacter    , os);
        ENUMCASEOS(FDSN_StatusInvalidBucketNameCharacter         , os);
        ENUMCASEOS(FDSN_StatusInvalidBucketNameCharacterSequence , os);
        ENUMCASEOS(FDSN_StatusInvalidBucketNameTooShort          , os);
        ENUMCASEOS(FDSN_StatusInvalidBucketNameDotQuadNotation   , os);
        ENUMCASEOS(FDSN_StatusQueryParamsTooLong                 , os);
        ENUMCASEOS(FDSN_StatusFailedToInitializeRequest          , os);
        ENUMCASEOS(FDSN_StatusMetaDataHeadersTooLong             , os);
        ENUMCASEOS(FDSN_StatusBadMetaData                        , os);
        ENUMCASEOS(FDSN_StatusBadContentType                     , os);
        ENUMCASEOS(FDSN_StatusContentTypeTooLong                 , os);
        ENUMCASEOS(FDSN_StatusBadMD5                             , os);
        ENUMCASEOS(FDSN_StatusMD5TooLong                         , os);
        ENUMCASEOS(FDSN_StatusBadCacheControl                    , os);
        ENUMCASEOS(FDSN_StatusCacheControlTooLong                , os);
        ENUMCASEOS(FDSN_StatusBadContentDispositionFilename      , os);
        ENUMCASEOS(FDSN_StatusContentDispositionFilenameTooLong  , os);
        ENUMCASEOS(FDSN_StatusBadContentEncoding                 , os);
        ENUMCASEOS(FDSN_StatusContentEncodingTooLong             , os);
        ENUMCASEOS(FDSN_StatusBadIfMatchETag                     , os);
        ENUMCASEOS(FDSN_StatusIfMatchETagTooLong                 , os);
        ENUMCASEOS(FDSN_StatusBadIfNotMatchETag                  , os);
        ENUMCASEOS(FDSN_StatusIfNotMatchETagTooLong              , os);
        ENUMCASEOS(FDSN_StatusHeadersTooLong                     , os);
        ENUMCASEOS(FDSN_StatusKeyTooLong                         , os);
        ENUMCASEOS(FDSN_StatusUriTooLong                         , os);
        ENUMCASEOS(FDSN_StatusXmlParseFailure                    , os);
        ENUMCASEOS(FDSN_StatusEmailAddressTooLong                , os);
        ENUMCASEOS(FDSN_StatusUserIdTooLong                      , os);
        ENUMCASEOS(FDSN_StatusUserDisplayNameTooLong             , os);
        ENUMCASEOS(FDSN_StatusGroupUriTooLong                    , os);
        ENUMCASEOS(FDSN_StatusPermissionTooLong                  , os);
        ENUMCASEOS(FDSN_StatusTargetBucketTooLong                , os);
        ENUMCASEOS(FDSN_StatusTargetPrefixTooLong                , os);
        ENUMCASEOS(FDSN_StatusTooManyGrants                      , os);
        ENUMCASEOS(FDSN_StatusBadGrantee                         , os);
        ENUMCASEOS(FDSN_StatusBadPermission                      , os);
        ENUMCASEOS(FDSN_StatusXmlDocumentTooLarge                , os);
        ENUMCASEOS(FDSN_StatusNameLookupError                    , os);
        ENUMCASEOS(FDSN_StatusFailedToConnect                    , os);
        ENUMCASEOS(FDSN_StatusServerFailedVerification           , os);
        ENUMCASEOS(FDSN_StatusConnectionFailed                   , os);
        ENUMCASEOS(FDSN_StatusAbortedByCallback                  , os);
        ENUMCASEOS(FDSN_StatusRequestTimedOut                    , os);
        ENUMCASEOS(FDSN_StatusEntityEmpty                        , os);
        ENUMCASEOS(FDSN_StatusEntityDoesNotExist                 , os);
        ENUMCASEOS(FDSN_StatusErrorAccessDenied                  , os);
        ENUMCASEOS(FDSN_StatusErrorAccountProblem                , os);
        ENUMCASEOS(FDSN_StatusErrorAmbiguousGrantByEmailAddress  , os);
        ENUMCASEOS(FDSN_StatusErrorBadDigest                     , os);
        ENUMCASEOS(FDSN_StatusErrorBucketAlreadyExists           , os);
        ENUMCASEOS(FDSN_StatusErrorBucketNotExists               , os);
        ENUMCASEOS(FDSN_StatusErrorBucketAlreadyOwnedByYou       , os);
        ENUMCASEOS(FDSN_StatusErrorBucketNotEmpty                , os);
        ENUMCASEOS(FDSN_StatusErrorCredentialsNotSupported       , os);
        ENUMCASEOS(FDSN_StatusErrorCrossLocationLoggingProhibited, os);
        ENUMCASEOS(FDSN_StatusErrorEntityTooSmall                , os);
        ENUMCASEOS(FDSN_StatusErrorEntityTooLarge                , os);
        ENUMCASEOS(FDSN_StatusErrorMissingContentLength          , os);
        ENUMCASEOS(FDSN_StatusErrorUnknown                       , os);
        default : os << "unknown status code (" << static_cast<int>(status) <<")";
    }
    return os;
}

std::string toString(FDSN_Status status) {
    switch (status) {
        ENUMCASE(FDSN_StatusNOTSET);
        ENUMCASE(ERR_OK);
        ENUMCASE(FDSN_StatusCreated);
        ENUMCASE(FDSN_StatusInternalError);
        ENUMCASE(FDSN_StatusOutOfMemory);
        ENUMCASE(FDSN_StatusInterrupted);
        ENUMCASE(FDSN_StatusTxnInProgress);
        ENUMCASE(FDSN_StatusInvalidBucketNameTooLong);
        ENUMCASE(FDSN_StatusInvalidBucketNameFirstCharacter);
        ENUMCASE(FDSN_StatusInvalidBucketNameCharacter);
        ENUMCASE(FDSN_StatusInvalidBucketNameCharacterSequence);
        ENUMCASE(FDSN_StatusInvalidBucketNameTooShort);
        ENUMCASE(FDSN_StatusInvalidBucketNameDotQuadNotation);
        ENUMCASE(FDSN_StatusQueryParamsTooLong);
        ENUMCASE(FDSN_StatusFailedToInitializeRequest);
        ENUMCASE(FDSN_StatusMetaDataHeadersTooLong);
        ENUMCASE(FDSN_StatusBadMetaData);
        ENUMCASE(FDSN_StatusBadContentType);
        ENUMCASE(FDSN_StatusContentTypeTooLong);
        ENUMCASE(FDSN_StatusBadMD5);
        ENUMCASE(FDSN_StatusMD5TooLong);
        ENUMCASE(FDSN_StatusBadCacheControl);
        ENUMCASE(FDSN_StatusCacheControlTooLong);
        ENUMCASE(FDSN_StatusBadContentDispositionFilename);
        ENUMCASE(FDSN_StatusContentDispositionFilenameTooLong);
        ENUMCASE(FDSN_StatusBadContentEncoding);
        ENUMCASE(FDSN_StatusContentEncodingTooLong);
        ENUMCASE(FDSN_StatusBadIfMatchETag);
        ENUMCASE(FDSN_StatusIfMatchETagTooLong);
        ENUMCASE(FDSN_StatusBadIfNotMatchETag);
        ENUMCASE(FDSN_StatusIfNotMatchETagTooLong);
        ENUMCASE(FDSN_StatusHeadersTooLong);
        ENUMCASE(FDSN_StatusKeyTooLong);
        ENUMCASE(FDSN_StatusUriTooLong);
        ENUMCASE(FDSN_StatusXmlParseFailure);
        ENUMCASE(FDSN_StatusEmailAddressTooLong);
        ENUMCASE(FDSN_StatusUserIdTooLong);
        ENUMCASE(FDSN_StatusUserDisplayNameTooLong);
        ENUMCASE(FDSN_StatusGroupUriTooLong);
        ENUMCASE(FDSN_StatusPermissionTooLong);
        ENUMCASE(FDSN_StatusTargetBucketTooLong);
        ENUMCASE(FDSN_StatusTargetPrefixTooLong);
        ENUMCASE(FDSN_StatusTooManyGrants);
        ENUMCASE(FDSN_StatusBadGrantee);
        ENUMCASE(FDSN_StatusBadPermission);
        ENUMCASE(FDSN_StatusXmlDocumentTooLarge);
        ENUMCASE(FDSN_StatusNameLookupError);
        ENUMCASE(FDSN_StatusFailedToConnect);
        ENUMCASE(FDSN_StatusServerFailedVerification);
        ENUMCASE(FDSN_StatusConnectionFailed);
        ENUMCASE(FDSN_StatusAbortedByCallback);
        ENUMCASE(FDSN_StatusRequestTimedOut);
        ENUMCASE(FDSN_StatusEntityEmpty);
        ENUMCASE(FDSN_StatusEntityDoesNotExist);
        ENUMCASE(FDSN_StatusErrorAccessDenied);
        ENUMCASE(FDSN_StatusErrorAccountProblem);
        ENUMCASE(FDSN_StatusErrorAmbiguousGrantByEmailAddress);
        ENUMCASE(FDSN_StatusErrorBadDigest);
        ENUMCASE(FDSN_StatusErrorBucketAlreadyExists);
        ENUMCASE(FDSN_StatusErrorBucketNotExists);
        ENUMCASE(FDSN_StatusErrorBucketAlreadyOwnedByYou);
        ENUMCASE(FDSN_StatusErrorBucketNotEmpty);
        ENUMCASE(FDSN_StatusErrorCredentialsNotSupported);
        ENUMCASE(FDSN_StatusErrorCrossLocationLoggingProhibited);
        ENUMCASE(FDSN_StatusErrorEntityTooSmall);
        ENUMCASE(FDSN_StatusErrorEntityTooLarge);
        ENUMCASE(FDSN_StatusErrorMissingContentLength);
        ENUMCASE(FDSN_StatusErrorUnknown);
        default:
        {
            return Error(status).GetErrstr();
        }
    }
}

FDSN_Status getStatusFromError(const Error& error) {
    switch (error.GetErrno()) {
        case ERR_OK                           : return ERR_OK;

        case ERR_DUPLICATE                    :
        case ERR_HASH_COLLISION               :
        case ERR_DISK_WRITE_FAILED            :
        case ERR_DISK_READ_FAILED             :
        case ERR_CAT_QUERY_FAILED             :
        case ERR_CAT_ENTRY_NOT_FOUND          :
        case ERR_INVALID_ARG                  :
        case ERR_PENDING_RESP                 : return FDSN_StatusInternalError; //NOLINT

        case ERR_VOL_ADMISSION_FAILED         :
        case ERR_GET_DLT_FAILED               :
        case ERR_GET_DMT_FAILED               :
        case ERR_NOT_IMPLEMENTED              :
        case ERR_OUT_OF_MEMORY                :
        case ERR_DUPLICATE_UUID               :
        case ERR_TRANS_JOURNAL_OUT_OF_IDS     :
        case ERR_TRANS_JOURNAL_REQUEST_QUEUED :
        case ERR_NODE_NOT_ACTIVE              :

        case ERR_NOT_READY                    :
        case ERR_INVALID_DLT                  :
        case ERR_PERSIST_STATE_MISMATCH       :
        case ERR_EXCEED_MIN_IOPS              : return FDSN_StatusInternalError; //NOLINT

        case ERR_UNAUTH_ACCESS                : return FDSN_StatusErrorAccessDenied; //NOLINT

        case ERR_NOT_FOUND                    :
        case ERR_BLOB_NOT_FOUND               : return FDSN_StatusEntityDoesNotExist; //NOLINT
        case ERR_VOL_NOT_FOUND                : return FDSN_StatusErrorBucketNotExists; //NOLINT
        case ERR_VOL_NOT_EMPTY                : return FDSN_StatusErrorBucketNotEmpty; //NOLINT

        default: return FDSN_StatusInternalError;
    }
}

Exception::Exception(const Error& e, const std::string& message) throw()
        : err(e), message(message) {
    if (message.empty()) {
        this->message = e.GetErrstr();
    }
}

Exception::Exception(const std::string& message) throw()
        : err(ERR_INVALID) , message(message) {
}

Exception::Exception(const Exception& e) throw() {
    err = e.err;
    message = e.message;
}

Exception& Exception::operator= (const Exception& e) throw() {
    err = e.err;
    message = e.message;
    return *this;
}

const Error& Exception::getError() const {
    return err;
}

Exception::~Exception() throw() {}

const char* Exception::what() const throw() {
    return message.c_str();
}

}  // namespace fds
