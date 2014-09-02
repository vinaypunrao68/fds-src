/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_SERIALIZE_H_
#define SOURCE_INCLUDE_SERIALIZE_H_

#include <netinet/in.h>
#include <boost/shared_ptr.hpp>
#include <shared/fds_types.h>
#include <fds_error.h>
#include <thrift/protocol/TProtocol.h>
#include <thrift/transport/TBufferTransports.h>
#include <util/timeutils.h>
#include <string>
#include <vector>
namespace fds {
namespace serialize {
typedef boost::shared_ptr<apache::thrift::protocol::TProtocol> TProtocolPtr;
/**
 * Helper class to serialize an object
 * has only WRITE methods
 */
struct Serializer {
    Serializer(TProtocolPtr proto);
    uint32_t writeBool(const bool value);
    uint32_t writeByte(const int8_t byte);
    uint32_t writeI16(const int16_t i16);
    uint32_t writeI32(const int32_t i32);
    uint32_t writeI64(const int64_t i64);
    uint32_t writeTimeStamp(const fds::util::TimeStamp timestamp);
    uint32_t writeDouble(const double dub);
    uint32_t writeString(const std::string& str);
    uint32_t writeBuffer(const int8_t* buf, const uint32_t& len);
    template <class T>
    uint32_t writeVector(const std::vector<T>& v);

    std::string getBufferAsString();
    virtual ~Serializer();
  private:
    TProtocolPtr proto;
};

/**
 * Helper class to de-serialize an object
 * has only READ methods
 */

struct Deserializer {
    Deserializer(TProtocolPtr proto);

    uint32_t readBool(bool& value);
    uint32_t readByte(int8_t& byte);
    uint32_t readI16(int16_t& i16);
    uint32_t readI32(int32_t& i32);
    uint32_t readI64(int64_t& i64);
    uint32_t readDouble(double& dub);
    uint32_t readString(std::string& str);

    // fds specific types
    uint32_t readByte(fds_uint8_t& ubyte);
    uint32_t readI16(fds_uint16_t& ui16);
    uint32_t readI32(fds_uint32_t& ui32);
    uint32_t readI64(fds_uint64_t& ui64);
    uint32_t readTimeStamp(fds::util::TimeStamp& timestamp);

    uint32_t readBuffer(int8_t* buf, const uint32_t& len);
    template <class T>
    uint32_t readVector(std::vector<T>& v);

    virtual ~Deserializer();
  private:
    TProtocolPtr proto;
};

/**
 * An class needs to implement these two methods to
 * serialize/deserialize .
 */
struct Serializable {
    uint32_t virtual write(Serializer*  s) const = 0;
    uint32_t virtual read(Deserializer* d) = 0;
    uint32_t virtual getEstimatedSize() const;
    Error virtual getSerialized(std::string& serializedData) const; //NOLINT
    Error virtual loadSerialized(const std::string& serializedData);
};

/**
 * In Memory
 */
Serializer* getMemSerializer(uint sz= 0);
Deserializer* getMemDeserializer(const std::string& str);

// File Based
Serializer* getFileSerializer(const std::string& filename);
Serializer* getFileSerializer(fds_int32_t fd);
Deserializer* getFileDeserializer(const std::string& filename);
Deserializer* getFileDeserializer(fds_int32_t fd);

template <class T>
static uint32_t getEstimatedSize(const std::vector<T>& v)
{
    return sizeof(int32_t) + (sizeof(T) * v.size());
}

template <class T>
uint32_t Serializer::writeVector(const std::vector<T>& v)
{
    /* Write size */
    uint32_t cnt = 0;
    cnt = writeI32(v.size());
    if (cnt == 0) {
        return cnt;
    }
    /* Write the vector buffer */
    if (v.size() > 0) {
        cnt += writeBuffer(reinterpret_cast<int8_t*>(const_cast<T*>(v.data())),
                           sizeof(T) * v.size());
    }
    return cnt;
}

template <class T>
uint32_t Deserializer::readVector(std::vector<T>& v)
{
    int32_t sz;
    /* read vector size */
    uint32_t ret = readI32(sz);
    if (ret == 0) {
        return 0;
    }

    /* Read the vector */
    v.resize(sz);
    if (sz > 0) {
        ret += readBuffer(reinterpret_cast<int8_t*>(const_cast<T*>(v.data())),
                          sizeof(T) * v.size());
    }

    return ret;
}
}  // namespace serialize
}  // namespace fds


#endif  // SOURCE_INCLUDE_SERIALIZE_H_
