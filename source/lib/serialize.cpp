
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

Serializer::Serializer(TProtocolPtr proto,TMemoryBuffer *memBuffer) : proto(proto),memBuffer(memBuffer) {
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
    if (!memBuffer) return "";
    return memBuffer->getBufferAsString();
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

Serializer* getMemSerializer(uint sz) {
    if (0==sz) sz=1024;
    TMemoryBuffer* memBuffer=new TMemoryBuffer(sz);
    boost::shared_ptr<TTransport> trans(memBuffer);
    TBinaryProtocol* proto = new TBinaryProtocol(trans);
    return new Serializer(TProtocolPtr (new TBinaryProtocol(trans)),memBuffer);
}

Deserializer* getMemDeserializer(std::string& str) {
    TMemoryBuffer *memBuffer = new TMemoryBuffer(0);
    memBuffer->resetBuffer((uint8_t*)const_cast<char*> (str.data()), (uint32_t)str.length());
    boost::shared_ptr<TTransport> trans(memBuffer);
    return new Deserializer(TProtocolPtr (new TBinaryProtocol(trans)));
}

} // namespace serialize
} // namespace fds
