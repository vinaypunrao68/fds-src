/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Object database class. The object database is a key-value store
 * that provides local objec storage.
 */

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
#include <fds_types.h>

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

    /* Globals */
    ObjectID NullObjectID;
    const int bitMaskLookup[64] = {0x0, 0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F, 0x7F, 0xFF,
                      0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF };

    ObjectID::ObjectID() {
        memset(digest, 0x00, sizeof(digest));
    }

    ObjectID::ObjectID(uint32_t dataSet) {
        memset(digest, dataSet, sizeof(digest));
    }

    ObjectID::ObjectID(uint8_t *objId) {
        memcpy(digest, objId, sizeof(digest));
    }

    ObjectID::ObjectID(uint8_t  *objId, uint32_t length) {
        memcpy(digest, objId, length);
    }


    ObjectID::ObjectID(const ObjectID& rhs) {
        memcpy(digest, rhs.digest, sizeof(digest));
    }

    ObjectID::ObjectID(const std::string& oid) {
        fds_verify(oid.size() == sizeof(digest));
        memcpy(digest, oid.c_str(), sizeof(digest));
    }

    ObjectID::~ObjectID() { }

    /*
     * Assumes the length of the data is 2 * hash size.
     * The caller needs to ensure this is the case.
     */
    void ObjectID::SetId(const char*data, fds_uint32_t len) {
      memcpy(digest, data, len);
    }

    void ObjectID::SetId(uint8_t  *data, fds_uint32_t  length) {
        memcpy(digest, data, length);
    }

    const uint8_t* ObjectID::GetId()  const {
        return digest;
    }

   /*
    * bit mask. this will help to boost the performance 
    */

    fds_uint64_t ObjectID::getTokenBits(fds_uint32_t numBits) const {
           fds_uint64_t tokenBits;
           int offset = 0;  //   pass this as argument
           int byteIndex = offset / 8;
           int bitIndex = offset % 8;
           int val;

           if ((bitIndex + numBits) > 16) {
              tokenBits = ((digest[byteIndex] << 16 | digest[byteIndex + 1]
                           << 8 | digest[byteIndex + 2])
                           >> (24 - bitIndex - numBits)) & fds::bitMaskLookup[numBits];
            } else if ((offset + numBits) > 8) {
              tokenBits = ((digest[byteIndex]
                           << 8 | digest[byteIndex + 1])
                           >> (16 - bitIndex - numBits)) & fds::bitMaskLookup[numBits];
           }else {
              tokenBits = (digest[byteIndex]
                        >> (8 - offset - numBits)) & fds::bitMaskLookup[numBits];
         }

      return tokenBits;
    }
    /*
     * Returns the size of the OID in bytes.
     */
    fds_uint32_t ObjectID::GetLen() const {
      return sizeof(digest);
    }

    /*
     * Returns a string representation.
     */
    std::string ObjectID::ToString() const {
        return ToHex();
    }

    /*
     * Operators
     */
    bool ObjectID::operator==(const ObjectID& rhs) const {
        return (compare(*this, rhs) == 0);
    }

    bool ObjectID::operator!=(const ObjectID& rhs) const {
        return (!operator==(rhs));
    }

    bool ObjectID::operator < (const ObjectID& rhs) const {
        return  compare(*this, rhs) < 0;
    }

    bool ObjectID::operator > (const ObjectID& rhs) const {
        return  compare(*this, rhs) > 0;
    }

    ObjectID& ObjectID::operator=(const ObjectID& rhs) {
      memcpy(digest, rhs.digest, sizeof(digest));
      return *this;
    }

    /**
     * Sets the object id based on a hex string
     * @param hexStr (i) Hex string
     *
     * Verifies the hex string and its format
     */
    ObjectID& ObjectID::operator=(const std::string& hexStr) {
        fds_verify(hexStr.compare(0, 2, "0x") == 0);  // Require 0x prefix
        fds_verify(hexStr.size() == (40 + 2));  // Account for 0x
        char a, b;
        uint j = 0;
        // Start the offset at 2 to account of 0x
        for (uint i = 2; i < hexStr.length(); i += 2, j++) {
           a = hexStr[i];   if (a > 57) a -= 87; else a -= 48; // NOLINT
           b = hexStr[i+1]; if (b > 57) b -= 87; else b -= 48; // NOLINT
           digest[j] =  (a << 4 & 0xF0) + b;
        }
        return *this;
    }

    std::string ObjectID::ToHex() const {
      return ToHex(*this);
    }

    /*
     * Static members for transforming ObjectIDs.
     * Just utility functions.
     */

    std::string ObjectID::ToHex(const ObjectID& oid) {
      std::ostringstream hash_oss;
      hash_oss << ToHex((const uint8_t *)oid.digest, oid.GetLen());
      return hash_oss.str();
    }

    std::string ObjectID::ToHex(const uint8_t *key, fds_uint32_t len) {
      std::ostringstream hash_oss;

      hash_oss << std::hex << std::setfill('0') << std::nouppercase;
      for (std::string::size_type i = 0; i < len; ++i) {
        hash_oss << std::setw(2) << (uint16_t)key[i];
      }

      return hash_oss.str();
    }

    std::string ObjectID::ToHex(const char *key, fds_uint32_t len) {
      std::ostringstream hash_oss;

      hash_oss << std::hex << std::setfill('0') << std::nouppercase;
      for (std::string::size_type i = 0; i < len; ++i) {
        hash_oss << std::setw(2) << (uint16_t)key[i];
      }
      return hash_oss.str();
    }

    std::string ObjectID::ToHex(const fds_uint32_t *key, fds_uint32_t len) {
      std::ostringstream hash_oss;

      hash_oss << std::hex << std::setfill('0') << std::nouppercase;
      for (std::string::size_type i = 0; i < len; ++i) {
        hash_oss << std::setw(8) << key[i];
      }
      return hash_oss.str();
    }

    int ObjectID::compare(const ObjectID &lhs, const ObjectID &rhs) {
        return  memcmp(lhs.digest, rhs.digest, sizeof(lhs.digest));
    }

    /* NullObjectID */
    extern ObjectID NullObjectID;

    std::ostream& operator<<(std::ostream& out, const ObjectID& oid) {
      return out << "Object ID: " << ObjectID::ToHex(oid);
    }

    fds_uint32_t str_to_ipv4_addr(std::string ip_str) {
      unsigned int n1, n2, n3, n4;
      fds_uint32_t ip;
      sscanf(ip_str.c_str(), "%d.%d.%d.%d", &n1, &n2, &n3, &n4); //  NOLINT
      ip = n1 << 24 | n2 << 16 | n3 << 8 | n4;
      return (ip);
    }

    std::string ipv4_addr_to_str(fds_uint32_t ip) {
      char tmp_ip[32];
      memset(tmp_ip, 0x00, sizeof(char) * 32); //  NOLINT
      sprintf(tmp_ip, "%u.%u.%u.%u", (ip >> 24), (ip >> 16)  //  NOLINT
                             & 0xff, (ip >> 8) & 0xff, ip & 0xff); //  NOLINT
      return (std::string(tmp_ip, strlen(tmp_ip)));
    }

    fds_bool_t FdsBlobReq::magicInUse() const {
      return (magic == FDS_SH_IO_MAGIC_IN_USE);
    }

    fds_volid_t FdsBlobReq::getVolId() const {
      return volId;
    }

    fds_io_op_t  FdsBlobReq::getIoType() const {
      return ioType;
    }

    void FdsBlobReq::setVolId(fds_volid_t vol_id) {
      volId = vol_id;
    }

    void FdsBlobReq::cbWithResult(int result) {
      return callback(result);
    }

    const std::string& FdsBlobReq::getBlobName() const {
      return blobName;
    }

    fds_uint64_t FdsBlobReq::getBlobOffset() const {
      return blobOffset;
    }

    const char* FdsBlobReq::getDataBuf() const {
      return dataBuf;
    }

    fds_uint64_t FdsBlobReq::getDataLen() const {
      return dataLen;
    }

    void FdsBlobReq::setDataLen(fds_uint64_t len) {
      dataLen = len;
    }

    void FdsBlobReq::setDataBuf(const char* _buf) {
      /*
       * TODO: For now we're assuming the buffer is preallocated
       * by the owner and the length has been set already.
       */
      memcpy(dataBuf, _buf, dataLen);
    }

    void FdsBlobReq::setObjId(const ObjectID& _oid) {
      objId = _oid;
    }

    void FdsBlobReq::setQueuedUsec(fds_uint64_t _usec) {
      queuedUsec = _usec;
    }

}  // namespace fds
