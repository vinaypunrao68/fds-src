/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_LIB_FDS_ERR_H_
#define SOURCE_LIB_FDS_ERR_H_

#include "lib/fds_types.h"

#include <string>

namespace fds {

  typedef enum {
    ERR_OK                = 0,
    ERR_DUPLICATE         = 1,
    ERR_DISK_WRITE_FAILED = 2,
    ERR_DISK_READ_FAILED  = 3,
  } fds_errno_t;

  class Error {
 private:
    fds_errno_t errno;
    std::string errstr;

 public:
    Error()
        : errno(ERR_OK) {
    }

    Error(fds_errno_t errno_arg)
        : errno(errno_arg) {
    }

    Error(Error& err)
        : errno(err.errno) {
    }

    Error& operator=(const Error& rhs) {
      errno = rhs.errno;
    }
    
    bool operator==(const fds_errno_t& rhs) {
      return (this->errno == rhs);
    }

    bool operator!=(const fds_errno_t& rhs) {
      return (this->errno != rhs);
    }

    ~Error() { }
  };
};  // namespace fds

#endif  // SOURCE_LIB_FDS_ERR_H_
