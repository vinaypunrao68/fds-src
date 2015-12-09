
#include <serialize.h>
#include <netinet/in.h>
#include <boost/shared_ptr.hpp>

#include <thrift/transport/TSimpleFileTransport.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <unistd.h>
#include <thrift/transport/TBufferTransports.h>
#include <fds_defines.h>
#include <util/Log.h>

using namespace apache::thrift::transport;
using namespace apache::thrift::protocol;

namespace fds { namespace serialize {

Serializer::Serializer(TProtocolPtr proto) : proto(proto) {
}

uint32_t Serializer::writeBool(const bool value) {
    return proto->writeBool(value);
}

uint32_t Serializer::writeByte(const int8_t byte) {
    return proto->writeByte(byte);
}

uint32_t Serializer::writeI16(const int16_t i16) {
    return proto->writeI16(i16);
}

uint32_t Serializer::writeI32(const int32_t i32){
    return proto->writeI32(i32);
}

uint32_t Serializer::writeI64(const int64_t i64) {
    return proto->writeI64(i64);
}

uint32_t Serializer::writeTimeStamp(const fds::util::TimeStamp timestamp) {
    return proto->writeI64(timestamp);
}

uint32_t Serializer::writeDouble(const double dub) {
    return proto->writeDouble(dub);
}

uint32_t Serializer::writeString(const std::string& str) {
    return proto->writeString(str);
}

uint32_t Serializer::writeBuffer(const int8_t* buf, uint32_t len) {
    proto->getTransport()->write(reinterpret_cast<const uint8_t*>(buf), len);
    return len;
}

std::string Serializer::getBufferAsString() {
    TMemoryBuffer *memBuffer = dynamic_cast<TMemoryBuffer*>(proto->getTransport().get());
    if (!memBuffer) return "";
    return memBuffer->getBufferAsString();
}

Serializer::~Serializer() {
    
}

Deserializer::Deserializer(TProtocolPtr proto) : proto(proto) {
}

uint32_t Deserializer::readBool(bool& value) {
    return proto->readBool(value);
}

uint32_t Deserializer::readByte(int8_t& byte) {
    return proto->readByte(byte);
}

uint32_t Deserializer::readI16(int16_t& i16) {
    return proto->readI16(i16);
}

uint32_t Deserializer::readI32(int32_t& i32) {
    return proto->readI32(i32);
}

uint32_t Deserializer::readI64(int64_t& i64) {
    return proto->readI64(i64);
}

uint32_t Deserializer::readDouble(double& dub)  {
    return proto->readDouble(dub);
}

uint32_t Deserializer::readString(std::string& str) {
    return proto->readString(str);
}

uint32_t Deserializer::readByte(fds_uint8_t& byte) {
    return proto->readByte((int8_t&)byte);
}

uint32_t Deserializer::readI16(fds_uint16_t& ui16) {
    return proto->readI16((int16_t&)ui16);
}

uint32_t Deserializer::readI32(fds_uint32_t& ui32) {
    return proto->readI32((int32_t&)ui32);
}

uint32_t Deserializer::readI64(fds_uint64_t& ui64) {
    return proto->readI64((int64_t&)ui64);
}

uint32_t Deserializer::readBuffer(int8_t* buf, uint32_t len)
{
    return proto->getTransport()->read(reinterpret_cast<uint8_t*>(buf), len);
}

uint32_t Deserializer::readTimeStamp(fds::util::TimeStamp& timestamp) {
    return proto->readI64((int64_t&)timestamp);
}

Deserializer::~Deserializer() {

}

uint32_t Serializable::getEstimatedSize() const {
    return 1*KB;
}

Error Serializable::getSerialized(std::string& serializedData) const {
    Error err(ERR_OK);
    uint32_t bytesWritten = 0;

    try {
        serialize::Serializer *s = serialize::getMemSerializer(getEstimatedSize());
        bytesWritten = write(s);
        serializedData.append(s->getBufferAsString());
        delete s;
    } catch(const TProtocolException& e) {
        LOGERROR << e.what();
        err = ERR_SERIALIZE_FAILED;
    } catch(...) {
        LOGERROR << "caught unexpected exception";
        err = ERR_SERIALIZE_FAILED;
    }
    if (bytesWritten == 0) {
        err = ERR_NO_BYTES_WRITTEN;
    }
    return err;
}

Error Serializable::loadSerialized(const std::string& serializedData) {
    Error err(ERR_OK);
    uint32_t bytesRead = 0;

    try {
        serialize::Deserializer *d = serialize::getMemDeserializer(serializedData);
        bytesRead = read(d);
        delete d;
    } catch(const TProtocolException& e) {
        LOGERROR << e.what();
        err = ERR_SERIALIZE_FAILED;
    } catch(...) {
        LOGERROR << "caught unexpected exception";
        err = ERR_SERIALIZE_FAILED;
    }
    if (bytesRead == 0) {
        err = ERR_NO_BYTES_READ;
    }
    return err;
}

Serializer* getMemSerializer(uint sz) {
    if (0==sz) sz=1024;
    TMemoryBuffer* memBuffer=new TMemoryBuffer(sz);
    boost::shared_ptr<TTransport> trans(memBuffer);
    return new Serializer(TProtocolPtr (new TBinaryProtocol(trans)));
}

Deserializer* getMemDeserializer(const std::string& str) {
    TMemoryBuffer *memBuffer = new TMemoryBuffer((uint8_t*)const_cast<char*> (str.data()), 
                                                 (uint32_t)str.length());
    boost::shared_ptr<TTransport> trans(memBuffer);
    return new Deserializer(TProtocolPtr (new TBinaryProtocol(trans)));
}

Serializer* getFileSerializer(const std::string& filename) {
    boost::shared_ptr<TTransport> trans(new TSimpleFileTransport(filename, false, true));
    return new Serializer(TProtocolPtr(new TBinaryProtocol(trans)));
}

Serializer* getFileSerializer(fds_int32_t fd) {
    boost::shared_ptr<TTransport> trans(new TFDTransport(fd));
    return new Serializer(TProtocolPtr(new TBinaryProtocol(trans)));
}

Deserializer* getFileDeserializer(const std::string& filename) {
    boost::shared_ptr<TTransport> trans(new TSimpleFileTransport(filename, true, false));
    return new Deserializer(TProtocolPtr(new TBinaryProtocol(trans)));
}

Deserializer* getFileDeserializer(fds_int32_t fd) {
    boost::shared_ptr<TTransport> trans(new TFDTransport(fd));
    return new Deserializer(TProtocolPtr(new TBinaryProtocol(trans)));
}
} // namespace serialize
} // namespace fds
