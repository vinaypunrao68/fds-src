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
#include <stdio.h>

#include <sstream>
#include <iostream>  // NOLINT(*)
#include <string>
#include <cstring>
#include <iomanip>
#include <ios>
#include <functional>
#include <shared/fds_types.h>

#include <fds_assert.h>

#include <boost/bind.hpp>
#include <boost/function.hpp>
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

  const int bitMaskLookup[64] = {0x0, 0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF };
  typedef fds_int64_t fds_volid_t;
  typedef fds_int64_t VolumeId;
  typedef fds_int64_t fds_qid_t;  // type for queue id

  /**
   * A token id identifies a particular token
   * region in a DLT routing table.
   */
  typedef fds_uint32_t fds_token_id;

  class ObjectID {
 private:
    uint8_t  digest[20];

 public:
    ObjectID() {
        memset(digest, 0x00, sizeof(digest));
    }

    ObjectID(uint32_t dataSet) {
        memset(digest, dataSet, sizeof(digest));
    }

    ObjectID(uint8_t *objId) {
        memcpy(digest, objId, sizeof(digest));
    }

    ObjectID(uint8_t  *objId, uint32_t length) {
        memcpy(digest, objId, length);
    }


    ObjectID(const ObjectID& rhs) {
        memcpy(digest, rhs.digest, sizeof(digest));
    }

    explicit ObjectID(const std::string& oid) {
        fds_verify(oid.size() == sizeof(digest));
        memcpy(digest, oid.c_str(), sizeof(digest));
    }

    ~ObjectID() { }



    /*
     * Assumes the length of the data is 2 * hash size.
     * The caller needs to ensure this is the case.
     */
    void SetId(const char*data, fds_uint32_t len) {
      memcpy(digest, data, len);
    }
    
    void SetId(uint8_t  *data, fds_uint32_t  length) {
        memcpy(digest, data, length);
    }

    void SetId(const std::string &data) {
        fds_assert(data.length() <= sizeof(digest));
        memcpy(digest, data.data(), data.length());
    }

    const uint8_t* GetId()  const {
	return digest;
    }

   /*
    * bit mask. this will help to boost the performance 
    */

     fds_uint64_t getTokenBits(fds_uint32_t numBits) const {
           fds_uint64_t tokenBits; 	
           int offset = 0;  //   pass this as argument 
           int byteIndex = offset / 8;
           int bitIndex = offset % 8;
           int val;

           if ((bitIndex + numBits) > 16) {
              tokenBits = ((digest[byteIndex] << 16 | digest[byteIndex + 1] << 8 | digest[byteIndex + 2])
                           >> (24 - bitIndex - numBits)) & fds::bitMaskLookup[numBits];
            } else if ((offset + numBits) > 8) {
              tokenBits = ((digest[byteIndex] << 8 | digest[byteIndex + 1])
                           >> (16 - bitIndex - numBits)) & fds::bitMaskLookup[numBits];
           }else {
              tokenBits = (digest[byteIndex] >> (8 - offset - numBits)) & fds::bitMaskLookup[numBits];
         }

     return tokenBits;
   }
    /*
     * Returns the size of the OID in bytes.
     */
    fds_uint32_t GetLen() const {
      return sizeof(digest);
    }

    /*
     * Returns a string representation.
     */
    std::string ToString() const {
        return ToHex();
    }

    /*
     * Operators
     */
    bool operator==(const ObjectID& rhs) const {
	return (compare(*this, rhs) == 0);
    }

    bool operator!=(const ObjectID& rhs) const {
	return (! operator==(rhs));
    }

    bool operator < (const ObjectID& rhs) const {
     return  compare(*this, rhs) < 0 ;
    }

    bool operator > (const ObjectID& rhs) const {
        return  compare(*this, rhs) > 0 ;
    }

    ObjectID& operator=(const ObjectID& rhs) {
      memcpy(digest, rhs.digest, sizeof(digest));
      return *this;
    }

    /**
     * Sets the object id based on a hex string
     * @param hexStr (i) Hex string
     *
     * Verifies the hex string and its format
     */
    ObjectID& operator=(const std::string& hexStr) {
        fds_verify(hexStr.compare(0, 2, "0x") == 0);  // Require 0x prefix
        fds_verify(hexStr.size() == (32 + 2));  // Account for 0x
        char a, b;
        uint j = 0;
        for (uint i = 0; i < hexStr.length(); i += 2,j++) {
           a = hexStr[i];   if (a > 57) a -= 87; else a -= 48; // NOLINT
           b = hexStr[i+1]; if (b > 57) b -= 87; else b -= 48; // NOLINT
           digest[j] =  (a << 4 & 0xF0) + b;
        }
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

//      hash_oss << ToHex((const char *)oid.digest,oid.GetLen());
      hash_oss << ToHex((const uint8_t *)oid.digest,oid.GetLen());

      return hash_oss.str();
    }

    static std::string ToHex(const uint8_t *key, fds_uint32_t len) {
      std::ostringstream hash_oss;

      hash_oss << std::hex << std::setfill('0') << std::nouppercase;
      for (std::string::size_type i = 0; i < len; ++i) {
        hash_oss << std::setw(2) << (uint16_t)key[i];
      }

      return hash_oss.str();
    }

    static std::string ToHex(const char *key, fds_uint32_t len) {
      std::ostringstream hash_oss;

      hash_oss << std::hex << std::setfill('0') << std::nouppercase;
      for (std::string::size_type i = 0; i < len; ++i) {
        hash_oss << std::setw(2) << (uint16_t)key[i];
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

    inline static int compare(const ObjectID &lhs, const ObjectID &rhs) {
        return  memcmp(lhs.digest, rhs.digest, sizeof(lhs.digest));
    }

    friend class ObjectLess;
    friend class ObjIdGen;
  };

  /* NullObjectID */
  extern ObjectID NullObjectID;

  inline std::ostream& operator<<(std::ostream& out, const ObjectID& oid) {
    return out << "Object ID: " << ObjectID::ToHex(oid);
  }
  
  class ObjectHash {
  public:
    size_t operator()(const ObjectID& oid) const {
      return std::hash<std::string>()(oid.ToHex());
    }
  };

  class ObjectLess {
  public:
    bool operator() (const ObjectID& oid1, const ObjectID& oid2) {
      return oid1 < oid2;
    }
  };

  struct DiskLoc {
    fds_uint64_t vol_id;
    fds_uint16_t file_id;
    fds_uint64_t offset;
  };

  class ObjectBuf { 
  public:
    fds_uint32_t size;
    std::string data;
    ObjectBuf()
      : size(0), data("")
      {
      }
    explicit ObjectBuf(const std::string &str)
    : data(str)
    {
        size = str.length();
    }
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
   FDS_LIST_BLOB,
   FDS_PUT_BLOB, 
   FDS_GET_BLOB,
   FDS_DELETE_BLOB,
   FDS_LIST_BUCKET,
   FDS_BUCKET_STATS,
   FDS_SM_READ_TOKEN_OBJECTS,
   FDS_SM_WRITE_TOKEN_OBJECTS,
   FDS_SM_SNAPSHOT_TOKEN,
   FDS_SM_SYNC_APPLY_METADATA,
   FDS_SM_SYNC_RESOLVE_SYNC_ENTRIES,
   FDS_OP_INVALID
  } fds_io_op_t;

#define  FDS_SH_IO_MAGIC_IN_USE 0x1B0A2C3D
#define  FDS_SH_IO_MAGIC_NOT_IN_USE 0xE1D0A2B3

  class FdsBlobReq {
 public:
    /*
     * Callback members
     * TODO: Resolve this with what's needed by the object-based callbacks.
     */
    typedef boost::function<void(fds_int32_t)> callbackBind;

  protected:
    /*
     * Common request header members
     */
    fds_uint32_t magic;
    fds_io_op_t  ioType;

    /*
     * Volume members
     */
    fds_volid_t volId;

    /*
     * Blob members
     */
    std::string  blobName;
    fds_uint64_t blobOffset;

    /*
     * Object members
     */
    ObjectID objId;

    /*
     * Buffer members
     */
    fds_uint64_t  dataLen;
    char         *dataBuf;

    /*
     * Callback members
     */
    callbackBind callback;    

    /*
     * Perf members
     */
    fds_uint64_t queuedUsec;  /* Time spec in queue */
    
 public:
    FdsBlobReq(fds_io_op_t      _op,
               fds_volid_t        _volId,
               const std::string &_blobName,
               fds_uint64_t       _blobOffset,
               fds_uint64_t       _dataLen,
               char              *_dataBuf)
        : magic(FDS_SH_IO_MAGIC_IN_USE),
        ioType(_op),
        volId(_volId),
        blobName(_blobName),
        blobOffset(_blobOffset),
        dataLen(_dataLen),
        dataBuf(_dataBuf) {
    }
    template<typename F, typename A, typename B, typename C>
    FdsBlobReq(fds_io_op_t      _op,
               fds_volid_t        _volId,
               const std::string &_blobName,
               fds_uint64_t       _blobOffset,
               fds_uint64_t       _dataLen,
               char              *_dataBuf,
               F f,
               A a,
               B b,
               C c)
        : magic(FDS_SH_IO_MAGIC_IN_USE),
        ioType(_op),
        volId(_volId),
        blobName(_blobName),
        blobOffset(_blobOffset),
        dataLen(_dataLen),
        dataBuf(_dataBuf),
        callback(boost::bind(f, a, b, c, _1)) {
    }
    ~FdsBlobReq() {
      magic = FDS_SH_IO_MAGIC_NOT_IN_USE;
    }

    fds_bool_t magicInUse() const {
      return (magic == FDS_SH_IO_MAGIC_IN_USE);
    }
    fds_volid_t getVolId() const {
      return volId;
    }
    fds_io_op_t  getIoType() const {
      return ioType;
    }
    void setVolId(fds_volid_t vol_id) {
    volId = vol_id;
    }
    void cbWithResult(int result) {
      return callback(result);
    }
    const std::string& getBlobName() const {
      return blobName;
    }
    fds_uint64_t getBlobOffset() const {
      return blobOffset;
    }
    const char *getDataBuf() const {
      return dataBuf;
    }
    fds_uint64_t getDataLen() const {
      return dataLen;
    }
    void setDataLen(fds_uint64_t len) {
      dataLen = len;
    }
    void setDataBuf(const char* _buf) {
      /*
       * TODO: For now we're assuming the buffer is preallocated
       * by the owner and the length has been set already.
       */
      memcpy(dataBuf, _buf, dataLen);
    }
    void setObjId(const ObjectID& _oid) {
      objId = _oid;
    }
    void setQueuedUsec(fds_uint64_t _usec) {
      queuedUsec = _usec;
    }
  };

  class FDS_IOType {
public:
   FDS_IOType() { };
  ~FDS_IOType() { };

   typedef enum {
    STOR_HV_IO,
    STOR_MGR_IO,
    DATA_MGR_IO
   } ioModule;

   int         io_magic;
   fds_io_op_t io_type;
   fds_uint32_t io_req_id;
   fds_volid_t io_vol_id;
   fds_int32_t io_status;
   fds_uint32_t io_service_time; //usecs
   fds_uint32_t io_wait_time; //usecs
   fds_uint32_t io_total_time; //usecs
   ioModule io_module; // IO belongs to which module for Qos proc 
   boost::posix_time::ptime enqueue_time;
   boost::posix_time::ptime dispatch_time;
   boost::posix_time::ptime io_done_time;	 

   /*
    * TODO: Blkdev specific fields that can be REMOVED.
    * These are left here simply because legacy, unused code
    * still references these and needs them to compile.
    */
   void *fbd_req;
   void *vbd;
   void *vbd_req;
   blkdev_complete_req_cb_t comp_req;
  };

}  // namespace fds

namespace std {
    template <>
    struct hash<fds::ObjectID> {
        std::size_t operator()(const fds::ObjectID oid) const {
            return std::hash<std::string>()(oid.ToHex());
        }
    };
}

#endif  // SOURCE_LIB_FDS_TYPES_H_
