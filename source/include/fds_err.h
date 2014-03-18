/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_LIB_FDS_ERR_H_
#define SOURCE_LIB_FDS_ERR_H_

#include <sstream>
#include <string>
#include <fds_types.h>
#include <fdsp/FDSP_types.h>

namespace fds {
  
  static const char* fds_errstrs[] = {
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
    "Volume is not  Empty"
  };
  
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
    ERR_OUT_OF_MEMEORY       = 14,
    ERR_DUPLICATE_UUID       = 15,
    ERR_TRANS_JOURNAL_OUT_OF_IDS = 16,
    ERR_TRANS_JOURNAL_REQUEST_QUEUED = 17,
    ERR_NODE_NOT_ACTIVE      = 18,
    ERR_VOL_NOT_EMPTY      = 19,

    /* I/O error range */
    ERR_IO_DLT_MISMATCH      = 100,

    /* Metadata error range */
    ERR_BLOB_OFFSET_INVALID  = 500,
    ERR_BLOB_NOT_FOUND       = 501,

    /* Migration error range [1000-1500) */
    ERR_MIGRATION_DUPLICATE_REQUEST = 1000,

    /* FdsActor err range [1500-2000) */
    ERR_FAR_INVALID_REQUEST = 1500,
    /* FDS actor is shutdown */
    ERR_FAR_SHUTDOWN = 1501,

    ERR_MAX
  } fds_errno_t;
  
  class Error {
 private:
    fds_errno_t _errno;
    /*
     * TODO: Change the local errstr to be a table
     * lookup rather than a member variable.
     */
    std::string errstr;

 public:
    Error()
        : _errno(ERR_OK),
        errstr(fds_errstrs[ERR_OK]) {
    }

    Error(fds_errno_t errno_arg)
        : _errno(errno_arg),
        errstr(fds_errstrs[errno_arg]) {
    }

    /**
     * Maps FDSP error number back to fds_errno.
     */
    Error(fds_uint32_t errno_fdsp)
            : _errno(static_cast<fds_errno_t>(errno_fdsp)) {
        errstr = fds_errstrs[_errno];
    }

    Error(const Error& err)
        : _errno(err._errno),
        errstr(err.errstr) {
    }

    bool OK() const {
      return _errno == ERR_OK;
    }

    bool ok() const {
      return OK();
    }

    fds_errno_t GetErrno() const {
      return _errno;
    }

    std::string GetErrstr() const {
      return errstr;
    }

    FDS_ProtocolInterface::FDSP_ErrType getFdspErr() const
    {
        switch (_errno) {
            case ERR_OK:
                return FDS_ProtocolInterface::FDSP_ERR_OKOK;
            case ERR_IO_DLT_MISMATCH:
                return FDS_ProtocolInterface::FDSP_ERR_DLT_CONFLICT;
            default:
                if (!OK()) {
		  return FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
                }
        }
        return FDS_ProtocolInterface::FDSP_ERR_OKOK;
    }

    Error& operator=(const Error& rhs) {
      _errno = rhs._errno;
      errstr = rhs.errstr;
      return *this;
    }

    Error& operator=(const fds_errno_t& rhs) {
      _errno = rhs;
      errstr = fds_errstrs[_errno];
      return *this;
    }

    bool operator==(const Error& rhs) const {
      return (this->_errno == rhs._errno);
    }

    bool operator==(const fds_errno_t& rhs) const {
      return (this->_errno == rhs);
    }

    bool operator!=(const Error& rhs) const {
      return (this->_errno != rhs._errno);
    }

    bool operator!=(const fds_errno_t& rhs) const {
      return (this->_errno != rhs);
    }

    ~Error() { }
  };
  
  inline std::ostream& operator<<(std::ostream& out, const Error& err) {
    return out << "Error " << err.GetErrno() << ": " << err.GetErrstr();
  }
};  // namespace fds

#endif  // SOURCE_LIB_FDS_ERR_H_
