/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <fds_err.h>
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
    "Object Buffer corrupted network",
    "OnDisk Object Buffer corrupt",
    "Dlt mismatch",
    "Invalid blob offset",
    "Duplicate migration request",
    "Invalid fds actor request",
    "fds actor has been shutdown",
    "Token is not sync mode",
    "SM object metadata not found",
    "error during network communication"
    "Unauthorized access to the object/bucket "
};


Error::Error() : _errno(ERR_OK) {
}

Error::Error(fds_errno_t errno_arg) : _errno(errno_arg) {
}

/**
 * Maps FDSP error number back to fds_errno.
 */
Error::Error(fds_uint32_t errno_fdsp) : _errno(static_cast<fds_errno_t>(errno_fdsp)) {
}

Error::Error(const Error& err) : _errno(err._errno) {
}

bool Error::OK() const {
    return _errno == ERR_OK;
}

bool Error::ok() const {
    return OK();
}

fds_errno_t Error::GetErrno() const {
    return _errno;
}

std::string Error::GetErrstr() const {
    if (_errno <= ERR_VOL_NOT_FOUND) {
        return fds_errstrs[_errno];
    }
    std::string ret = "Error no: " + std::to_string(_errno);
    return ret;
}

FDS_ProtocolInterface::FDSP_ErrType Error::getFdspErr() const {
    switch (_errno) {
        case ERR_OK:
            return FDS_ProtocolInterface::FDSP_ERR_OKOK;
        default:
            if (!OK()) {
                return FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
            }
    }
    return FDS_ProtocolInterface::FDSP_ERR_OKOK;
}

Error& Error::operator=(const Error& rhs) {
    _errno = rhs._errno;
    return *this;
}

Error& Error::operator=(const fds_errno_t& rhs) {
    _errno = rhs;
    return *this;
}

bool Error::operator==(const Error& rhs) const {
    return (this->_errno == rhs._errno);
}

bool Error::operator==(const fds_errno_t& rhs) const {
    return (this->_errno == rhs);
}

bool Error::operator!=(const Error& rhs) const {
    return (this->_errno != rhs._errno);
}

bool Error::operator!=(const fds_errno_t& rhs) const {
    return (this->_errno != rhs);
}

Error::~Error() {
}

std::ostream& operator<<(std::ostream& out, const Error& err) {
    return out << "Error " << err.GetErrno() << ": " << err.GetErrstr();
}

std::ostream& operator<<(std::ostream& os, FDSN_Status status) {
    switch (status) {
        ENUMCASEOS(FDSN_StatusOK                                 , os);
        ENUMCASEOS(FDSN_StatusCreated                            , os);
        ENUMCASEOS(FDSN_StatusInternalError                      , os);
        ENUMCASEOS(FDSN_StatusOutOfMemory                        , os);
        ENUMCASEOS(FDSN_StatusInterrupted                        , os);
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
}  // namespace fds
