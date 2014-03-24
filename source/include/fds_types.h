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
#include <limits>

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

  typedef fds_int64_t fds_volid_t;
  typedef fds_int64_t VolumeId;
  typedef fds_int64_t fds_qid_t;  // type for queue id

  /**
   * A token id identifies a particular token
   * region in a DLT routing table.
   */
  typedef fds_uint32_t fds_token_id;

  /**
   * A blob version identifies a unique
   * version instance of a blob.
   */
  typedef fds_uint64_t blob_version_t;

  /**
   * Blob versions cannot be 0. That value will represent
   * either a null or uninitialized version.
   * The first valid version is 1.
   */
  static const blob_version_t blob_version_invalid = 0;
  static const blob_version_t blob_version_initial = 1;
  static const blob_version_t blob_version_deleted =
          std::numeric_limits<unsigned long long>::max();

  class ObjectID {
 private:
    uint8_t  digest[20];

 public:
    ObjectID();
    ObjectID(uint32_t dataSet);
    ObjectID(uint8_t *objId);
    ObjectID(uint8_t  *objId, uint32_t length);
    ObjectID(const ObjectID& rhs);
    explicit ObjectID(const std::string& oid);
    ~ObjectID();

    /*
     * Assumes the length of the data is 2 * hash size.
     * The caller needs to ensure this is the case.
     */
    void SetId(const char*data, fds_uint32_t len);
    void SetId(uint8_t  *data, fds_uint32_t  length);
    void SetId(const std::string &data) {
        fds_assert(data.length() <= sizeof(digest));
        memcpy(digest, data.data(), data.length());
    }
    const uint8_t* GetId() const;

   /*
    * bit mask. this will help to boost the performance 
    */

    fds_uint64_t getTokenBits(fds_uint32_t numBits) const; 
    fds_uint32_t GetLen() const;
    std::string ToString() const;
    bool operator==(const ObjectID& rhs) const;
    bool operator!=(const ObjectID& rhs) const;
    bool operator < (const ObjectID& rhs) const;
    bool operator > (const ObjectID& rhs) const;
    ObjectID& operator=(const ObjectID& rhs) ;
    ObjectID& operator=(const std::string& hexStr);
    std::string ToHex() const;
    static std::string ToHex(const ObjectID& oid);
    static std::string ToHex(const uint8_t *key, fds_uint32_t len);
    static std::string ToHex(const char *key, fds_uint32_t len);
    static std::string ToHex(const fds_uint32_t *key, fds_uint32_t len);
    static int compare(const ObjectID &lhs, const ObjectID &rhs);
    static void getTokenRange(const fds_token_id& token,
            const uint32_t& nTokenBits,
            ObjectID &start, ObjectID &end);

    friend class ObjectLess;
    friend class ObjIdGen;
  };

  /* NullObjectID */
  extern ObjectID NullObjectID;

  /* Operators on ObjectID */
  std::ostream& operator<<(std::ostream& out, const ObjectID& oid);

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
    void resize(const size_t &sz)
    {
        size = sz;
        data.resize(sz);
    }
  };


  fds_uint32_t str_to_ipv4_addr(std::string ip_str);
  std::string ipv4_addr_to_str(fds_uint32_t ip);

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
   FDS_SM_SYNC_RESOLVE_SYNC_ENTRY,
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

    fds_bool_t magicInUse() const;
    fds_volid_t getVolId() const;
    fds_io_op_t  getIoType() const;
    void setVolId(fds_volid_t vol_id);
    void cbWithResult(int result);
    const std::string& getBlobName() const;
    fds_uint64_t getBlobOffset() const;
    const char *getDataBuf() const;
    fds_uint64_t getDataLen() const;
    void setDataLen(fds_uint64_t len);
    void setDataBuf(const char* _buf);
    void setObjId(const ObjectID& _oid);
    void setQueuedUsec(fds_uint64_t _usec);
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

/*
 * NOTE!!! include only std typedefs here. Dont use any fds objects !!!!
 */
#include <string>
#include <vector>

namespace fds {
    // new c++11 typedef convention - pretty cool !!!
    using StringList  = std::vector<std::string> ;
    using ConstString = const std::string& ;
}  // namespace fds

#endif  // SOURCE_LIB_FDS_TYPES_H_
