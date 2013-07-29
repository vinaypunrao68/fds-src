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

#include <sstream>
#include <iostream>  // NOLINT(*)
#include <string>
#include <cstring>
#include <iomanip>
#include <ios>

/*
 * Consider encapsulating in the global
 * fds namespace.
 */

// FDS definitions of variable types
typedef uint32_t fds_uint32_t;
typedef int32_t  fds_int32_t;
typedef uint16_t fds_uint16_t;
typedef int16_t  fds_int16_t;
typedef uint8_t  fds_uint8_t;
typedef int8_t   fds_int8_t;
typedef char     fds_char_t;
typedef bool     fds_bool_t;
typedef uint64_t fds_uint64_t;
typedef uint64_t fds_int64_t;

/**
 * In-memory representation of an object ID
 */
namespace fds {

  class ObjectID {
 private:
    fds_uint64_t hash_high;
    fds_uint64_t hash_low;

 public:
    ObjectID()
        : hash_high(0),
        hash_low(0) {
    }

    ObjectID(fds_uint64_t high, fds_uint64_t low)
        : hash_high(high),
        hash_low(low) {
    }

    explicit ObjectID(const std::string& oid) {
      memcpy(&hash_high,
             oid.c_str(),
             sizeof(fds_uint64_t));
      memcpy(&hash_low,
             oid.c_str() + sizeof(fds_uint64_t),
             sizeof(fds_uint64_t));
    }

    ~ObjectID();

    static std::string ToHex(const ObjectID& oid) {
      std::ostringstream hash_oss;

      hash_oss << ToHex(&(oid.hash_high), 1) << ToHex(&(oid.hash_low), 1);

      return hash_oss.str();
    }

    static std::string ToHex(const fds_uint64_t *key, fds_uint32_t len) {
      std::ostringstream hash_oss;

      hash_oss << std::hex << std::setfill('0') << std::nouppercase;
      for (std::string::size_type i = 0; i < len; ++i) {
        hash_oss << std::setw(16) << key[i];
      }

      return hash_oss.str();
    }

    static std::string ToHex(const fds_uint32_t *key, fds_uint32_t len) {
      std::ostringstream hash_oss;

      hash_oss << std::hex << std::setfill('0') << std::nouppercase;
      for (std::string::size_type i = 0; i < len; ++i) {
        hash_oss << std::setw(8) << key[i];
      }

      return hash_oss.str();
    }

    static std::string ToHex(const std::string& key) {
      std::ostringstream hash_oss;

      hash_oss << std::hex << std::setfill('0') << std::nouppercase;
      for (std::string::size_type i = 0; i < key.size(); ++i) {
        hash_oss << std::setw(2) << static_cast<int>(key[i]);
      }

      return hash_oss.str();
    }
  };

  inline std::ostream& operator<<(std::ostream& out, const ObjectID& oid) {
    return out << "Object ID: " << ObjectID::ToHex(oid);
  }

struct DiskLoc {
  fds_uint16_t vol_id;
  fds_uint16_t file_id;
  fds_uint32_t offset;
};

struct ObjectBuf {
  fds_uint32_t size;
  std::string data;
};

}  // namespace fds

#endif  // SOURCE_LIB_FDS_TYPES_H_
