/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Object database class. The object database is a key-value store
 * that provides local objec storage.
 */
#ifndef SOURCE_LIB_FDS_TYPES_H_
#define SOURCE_LIB_FDS_TYPES_H_

#include <stdint.h>

/*
 * Consider encapsulating in the global
 * fds namespace.
 */

// FDS definitions of variable types
typedef uint32_t fds_uint32_t;
typedef int32_t  fds_int32_t;
typedef uint16_t fds_uint16_t;
typedef int16_t  fds_int16_t;
typedef char     fds_char_t;
typedef bool     fds_bool_t;
typedef uint64_t fds_uint64_t;
typedef uint64_t fds_int64_t;

struct ObjectID {
};

struct DiskLoc {
  fds_uint16_t vol_id;
  fds_uint16_t file_id;
  fds_uint32_t offset;
};

struct ObjectBuf {
  fds_uint32_t size;
  std::string data;
};

#endif  // SOURCE_LIB_FDS_TYPES_H_
