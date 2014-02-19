
#include <serialize.h>
#include <netinet/in.h>
#include <boost/shared_ptr.hpp>

#include <thrift/transport/TSimpleFileTransport.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <unistd.h>
#include <thrift/transport/TBufferTransports.h>

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

uint32_t Serializer::writeDouble(const double dub) {
    return proto->writeDouble(dub);
}

uint32_t Serializer::writeString(const std::string& str) {
    return proto->writeString(str);
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

Deserializer::~Deserializer() {

}

Serializer* getMemSerializer(uint sz) {
    if (0==sz) sz=1024;
    TMemoryBuffer* memBuffer=new TMemoryBuffer(sz);
    boost::shared_ptr<TTransport> trans(memBuffer);
    return new Serializer(TProtocolPtr (new TBinaryProtocol(trans)));
}

Deserializer* getMemDeserializer(std::string& str) {
    TMemoryBuffer *memBuffer = new TMemoryBuffer((uint8_t*)const_cast<char*> (str.data()), 
                                                 (uint32_t)str.length());
    boost::shared_ptr<TTransport> trans(memBuffer);
    return new Deserializer(TProtocolPtr (new TBinaryProtocol(trans)));
}

Serializer* getFileSerializer(const std::string& filename) {
    boost::shared_ptr<TTransport> trans(new TSimpleFileTransport(filename, false, true));
    return new Serializer(TProtocolPtr(new TBinaryProtocol(trans)));
}

Deserializer* getFileDeserializer(const std::string& filename) {
    boost::shared_ptr<TTransport> trans(new TSimpleFileTransport(filename, true, false));
    return new Deserializer(TProtocolPtr(new TBinaryProtocol(trans)));
}

} // namespace serialize
} // namespace fds
