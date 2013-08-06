/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_LIB_FDS_ERR_H_
#define SOURCE_LIB_FDS_ERR_H_

#include "fds_types.h"

#include <sstream>
#include <string>

namespace fds {
  
  static const char* fds_errstrs[] = {
    "ALL OK",
    "Data is a duplicate",
    "Unable to write data to disk",
    "Unable to read data from disk",
  };
  
  typedef enum {
    ERR_OK                = 0,
    ERR_DUPLICATE         = 1,
    ERR_DISK_WRITE_FAILED = 2,
    ERR_DISK_READ_FAILED  = 3,
    ERR_MAX
  } fds_errno_t;
  
  class Error {
 private:
    fds_errno_t fds_errno;
    std::string fds_errstr;

 public:
    Error() : fds_errno(ERR_OK),fds_errstr(fds_errstrs[ERR_OK]) {
    }

    Error(fds_errno_t errno_arg)
        : fds_errno(errno_arg),
        fds_errstr(fds_errstrs[errno_arg]) {
    }

    Error(Error& err)
        : fds_errno(err.fds_errno),
        fds_errstr(err.fds_errstr) {
    }

    fds_errno_t GetErrno() const {
      return fds_errno;
    }

    std::string GetErrstr() const {
      return fds_errstr;
    }

    Error& operator=(const Error& rhs) {
      fds_errno = rhs.fds_errno;
      return *this;
    }

    bool operator==(const fds_errno_t& rhs) {
      return (this->fds_errno == rhs);
    }

    bool operator!=(const fds_errno_t& rhs) {
      return (this->fds_errno != rhs);
    }

    ~Error() { }
  };
  
  inline std::ostream& operator<<(std::ostream& out, const Error& err) {
    return out << "Error " << err.GetErrno() << ": " << err.GetErrstr();
  }
};  // namespace fds

#endif  // SOURCE_LIB_FDS_ERR_H_
