/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_LIB_FDS_ERR_H_
#define SOURCE_LIB_FDS_ERR_H_

#include <sstream>
#include <string>

namespace fds {
  
  static const char* fds_errstrs[] = {
    "ALL OK",
    "Data is a duplicate",
    "Unable to write data to disk",
    "Unable to read data from disk",
    "Unable to query into catalog",
    "Catalog entry not found",
    "Invalid argument or parameter",
  };
  
  typedef enum {
    ERR_OK                = 0,
    ERR_DUPLICATE         = 1,
    ERR_DISK_WRITE_FAILED = 2,
    ERR_DISK_READ_FAILED  = 3,
    ERR_CAT_QUERY_FAILED  = 4,
    ERR_CAT_ENTRY_NOT_FOUND  = 5,
    ERR_INVALID_ARG       = 6,
    ERR_PENDING_RESP      = 7,
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

    Error(Error& err)
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

    bool operator==(const Error& rhs) {
      return (this->_errno == rhs._errno);
    }

    bool operator==(const fds_errno_t& rhs) {
      return (this->_errno == rhs);
    }

    bool operator!=(const Error& rhs) {
      return (this->_errno != rhs._errno);
    }

    bool operator!=(const fds_errno_t& rhs) {
      return (this->_errno != rhs);
    }

    ~Error() { }
  };
  
  inline std::ostream& operator<<(std::ostream& out, const Error& err) {
    return out << "Error " << err.GetErrno() << ": " << err.GetErrstr();
  }
};  // namespace fds

#endif  // SOURCE_LIB_FDS_ERR_H_
