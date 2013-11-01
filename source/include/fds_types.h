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
#include <functional>
#include <shared/fds_types.h>

#include <boost/date_time/posix_time/posix_time.hpp>

/*
 * Consider encapsulating in the global
 * fds namespace.
 */

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

  typedef fds_uint64_t fds_volid_t;

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

    ObjectID(const ObjectID& rhs)
        : hash_high(rhs.hash_high),
        hash_low(rhs.hash_low) {
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
     * Assumes the length of the data is 2 * hash size.
     * The caller needs to ensure this is the case.
     */
    void SetId(const char* data) {
      memcpy(&hash_high, data, sizeof(fds_uint64_t));
      memcpy(&hash_low, data + sizeof(fds_uint64_t), sizeof(fds_uint64_t));
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
    bool operator==(const ObjectID& rhs) const {
      return ((this->hash_high == rhs.hash_high) &&
              (this->hash_low == rhs.hash_low));
    }

    bool operator!=(const ObjectID& rhs) const {
      return !(*this == rhs);
    }

    ObjectID& operator=(const ObjectID& rhs) {
      hash_high = rhs.hash_high;
      hash_low = rhs.hash_low;
      return *this;
    }

    std::string ToHex() const {
      return ToHex(*this);
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
  
  class ObjectHash {
 public:
    size_t operator()(const ObjectID& oid) const {
      return std::hash<std::string>()(oid.ToHex());
    }
  };

  struct DiskLoc {
    fds_uint64_t vol_id;
    fds_uint16_t file_id;
    fds_uint64_t offset;
  };

  struct ObjectBuf {
    fds_uint32_t size;
    std::string data;
  };

  inline fds_uint32_t str_to_ipv4_addr(std::string ip_str) {
    
    unsigned int n1, n2, n3, n4;
    fds_uint32_t ip;
    sscanf(ip_str.c_str(), "%d.%d.%d.%d", &n1, &n2, &n3, &n4);
    ip = n1 << 24 | n2 << 16 | n3 << 8 | n4;
    return (ip);
  }

  inline std::string ipv4_addr_to_str(fds_uint32_t ip) {
    
    char tmp_ip[32];
    memset(tmp_ip, 0x00, sizeof(char) * 32);
    sprintf(tmp_ip, "%u.%u.%u.%u", (ip >> 24), (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);
    return (std::string(tmp_ip, strlen(tmp_ip)));

  }

  typedef void (*blkdev_complete_req_cb_t)(void *arg1, void *arg2, void *treq, int res);
  typedef enum {
   FDS_IO_READ,
   FDS_IO_WRITE,
   FDS_IO_REDIR_READ,
   FDS_IO_OFFSET_WRITE,
   FDS_CAT_UPD,
   FDS_CAT_QRY,
   FDS_OP_INVALID
  } fds_io_op_t;

  class FDS_IOType {
public:
   FDS_IOType() { };
  ~FDS_IOType() { };

   typedef enum {
    STOR_HV_IO,
    STOR_MGR_IO,
    DATA_MGR_IO
   } ioModule;

   fds_io_op_t io_type;
   fds_uint32_t io_req_id;
   fds_volid_t io_vol_id;
   fds_int32_t io_status;
   fds_uint32_t io_service_time; //usecs
   fds_uint32_t io_wait_time; //usecs
   ioModule io_module; // IO belongs to which module for Qos proc 
   boost::posix_time::ptime enqueue_time;
   boost::posix_time::ptime dispatch_time;
   boost::posix_time::ptime io_done_time;	 

 
   // SM & DM objects for IO processing
   //  //FDSP_MsgHdrTypePtr msgHdr;
   //   //FDSP_PutObjTypePtr putObj;
   //    //FDSP_GetObjTypePtr getObj;
   //
   //     // SH & UBD objects for IO processing -  blockdevice mode
   void *fbd_req;
   void *vbd;
   void *vbd_req;
   blkdev_complete_req_cb_t comp_req;
  };

}  // namespace fds

#endif  // SOURCE_LIB_FDS_TYPES_H_
