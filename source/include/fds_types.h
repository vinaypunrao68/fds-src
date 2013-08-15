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
typedef uint32_t  fds_uint32_t;
typedef int32_t   fds_int32_t;
typedef uint16_t  fds_uint16_t;
typedef int16_t   fds_int16_t;
typedef uint8_t   fds_uint8_t;
typedef int8_t    fds_int8_t;
typedef char      fds_char_t;
typedef bool      fds_bool_t;
typedef uint64_t  fds_uint64_t;
typedef long      fds_long_t;

/*
 * UDP & TCP port numbers reserver for
 * StorMgr and DataMgr servers.
 */
#define FDS_CLUSTER_TCP_PORT_SM         6900
#define FDS_CLUSTER_TCP_PORT_DM         6901
#define FDS_CLUSTER_UDP_PORT_SM         9600
#define FDS_CLUSTER_UDP_PORT_DM         9601

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

    ~ObjectID() { }

    fds_uint64_t GetHigh() const {
      return hash_high;
    }

    fds_uint64_t GetLow() const {
      return hash_low;
    }

    void SetId(fds_uint64_t _high, fds_uint64_t _low) {
      hash_high = _high;
      hash_low  = _low;
    }
    
    /*
     * Returns the size of the OID in bytes.
     */
    fds_uint32_t GetLen() const {
      return sizeof(hash_high) + sizeof(hash_low);
    }

    /*
     * Returns a string representation.
     */
    std::string ToString() const {
      char str[GetLen()];
      memcpy(str, &hash_high, sizeof(fds_uint64_t));
      memcpy(str + sizeof(fds_uint64_t), &hash_low, sizeof(fds_uint64_t));

      return std::string(str, GetLen());
    }

    /*
     * Operators
     */
    bool operator==(const ObjectID& rhs) {
      return ((this->hash_high == rhs.hash_high) &&
              (this->hash_low == rhs.hash_low));
    }

    bool operator!=(const ObjectID& rhs) {
      return !(*this == rhs);
    }

    ObjectID& operator=(const ObjectID& rhs) {
      hash_high = rhs.hash_high;
      hash_low = rhs.hash_low;
      return *this;
    }

    /*
     * Static members for transforming ObjectIDs.
     * Just utility functions.
     */
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
