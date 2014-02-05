#ifndef SOURCE_INCLUDE_SERIALIZE_SERIALIZER_H_
#define SOURCE_INCLUDE_SERIALIZE_SERIALIZER_H_

#include <netinet/in.h>
#include <boost/shared_ptr.hpp>
#include <fds_types.h>
#include <thrift/protocol/TProtocol.h>
#include <thrift/transport/TBufferTransports.h>

namespace fds { 
    namespace serialize {
        typedef boost::shared_ptr<apache::thrift::protocol::TProtocol> TProtocolPtr;
        /**
         * Helper class to serialize an object
         * has only WRITE methods
         */
        struct Serializer {        
            Serializer(TProtocolPtr proto,apache::thrift::transport::TMemoryBuffer *memBuffer=NULL);
            
            uint32_t writeBool(const bool value);
            uint32_t writeByte(const int8_t byte);
            uint32_t writeI16(const int16_t i16);
            uint32_t writeI32(const int32_t i32);
            uint32_t writeI64(const int64_t i64);
            uint32_t writeDouble(const double dub);
            uint32_t writeString(const std::string& str);
            std::string getBufferAsString();

          private:
            TProtocolPtr proto;
            apache::thrift::transport::TMemoryBuffer *memBuffer;
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
            uint32_t readI32(int32_t& i32) ;
            uint32_t readI64(int64_t& i64) ;
            uint32_t readDouble(double& dub);
            uint32_t readString(std::string& str) ;

            // fds specific types
            uint32_t readByte(fds_uint8_t& ubyte);
            uint32_t readI16(fds_uint16_t& ui16);
            uint32_t readI32(fds_uint32_t& ui32);
            uint32_t readI64(fds_uint64_t& ui64);

          private:
            TProtocolPtr proto;
        };
        
        /**
         * An class needs to implement these two methods to
         * serialize/deserialize .
         */
        struct Serializable {            
            uint32_t virtual write(Serializer*  serializer  ) = 0;
            uint32_t virtual read(Deserializer* deserializer) = 0;
        };
        
        /**
         * In Memory 
         */
        Serializer* getMemSerializer(uint sz=0);
        Deserializer* getMemDeserializer(std::string& str);
    }
}


#endif
