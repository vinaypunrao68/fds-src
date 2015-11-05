/*
 * Copyright 2014-2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_FDSP_UTILS_H_
#define SOURCE_INCLUDE_FDSP_UTILS_H_

#include <string>
#include <unistd.h>
#include <exception>
#include <arpa/inet.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <fds_types.h>
#include <fdsp/svc_types_types.h>
#include <persistent-layer/dm_metadata.h>
#include <boost/make_shared.hpp>
#include <util/fiu_util.h>

/**
 * Maps FDSPMsg type to FDSPMsgTypeId enum
 */
#define FDSP_MSG_TYPEID(FDSPMsgT) FDSPMsgT##TypeId

#define MSG_DESERIALIZE(msgtype, error, payload) \
    fds::deserializeFdspMsg<fpi::msgtype>(const_cast<Error&>(error), payload)


// Forward declarations
namespace apache { namespace thrift { namespace transport {
    class TSocket;
    class TTransport;
}}}  // namespace apache::thrift::transport

namespace FDS_ProtocolInterface {
    class AsyncHdr;
    class SvcUuid;
}  // namespace FDS_ProtocolInterface

namespace fpi = FDS_ProtocolInterface;

namespace fds {
/* Forward declarations */
namespace bo  = boost;
namespace tt  = apache::thrift::transport;
namespace tp  = apache::thrift::protocol;
class ResourceUUID;

FDS_ProtocolInterface::FDS_ObjectIdType&
assign(FDS_ProtocolInterface::FDS_ObjectIdType& lhs, const ObjectID& rhs);

FDS_ProtocolInterface::FDS_ObjectIdType&
assign(FDS_ProtocolInterface::FDS_ObjectIdType& lhs, const meta_obj_id_t& rhs);

FDS_ProtocolInterface::FDS_ObjectIdType strToObjectIdType(const std::string & rhs);

FDS_ProtocolInterface::SvcUuid&
assign(FDS_ProtocolInterface::SvcUuid& lhs, const ResourceUUID& rhs);

FDS_ProtocolInterface::FDSP_Uuid&
assign(FDS_ProtocolInterface::FDSP_Uuid& lhs, const fpi::SvcID& rhs);

void swapAsyncHdr(boost::shared_ptr<fpi::AsyncHdr> &header);

std::string logString(const FDS_ProtocolInterface::AsyncHdr &header);

std::string quoteString(std::string const& text,
                        std::string const& delimiter = "\"",
                        std::string const& escape = "\\");

/**
* @brief For serializing FDSP messages
*
* @tparam PayloadT
* @param payload
* @param payloadBuf
*/
template<class PayloadT>
void serializeFdspMsg(const PayloadT &payload, bo::shared_ptr<std::string> &payloadBuf)
{
    uint32_t bufSz = 512;
    bo::shared_ptr<tt::TMemoryBuffer> buffer(new tt::TMemoryBuffer(bufSz));
    bo::shared_ptr<tp::TProtocol> binary_buf(new tp::TBinaryProtocol(buffer));
    try {
        auto written = payload.write(binary_buf.get());
        fds_verify(written > 0);
    } catch(std::exception &e) {
        /* This is to ensure we assert on any serialization exceptions in debug
         * builds.  We then rethrow the exception
         */
        GLOGDEBUG << "Excpetion in serializing: " << e.what();
        fds_assert(!"Exception serializing.  Most likely due to fdsp msg id mismatch");
        throw;
    } catch(...) {
        /* This is to ensure we assert on any serialization exceptions in debug
         * builds.  We then rethrow the exception
         */
        DBG(std::exception_ptr eptr = std::current_exception());
        fds_assert(!"Exception serializing.  Most likely due to fdsp msg id mismatch");
        throw;
    }
    payloadBuf = bo::make_shared<std::string>();
    *payloadBuf = buffer->getBufferAsString();
}

template<class PayloadT>
bo::shared_ptr<std::string> serializeFdspMsg(const PayloadT &payload)
{
    bo::shared_ptr<std::string> payloadBuf;
    serializeFdspMsg<PayloadT>(payload, payloadBuf);
    return payloadBuf;
}

/**
* @brief For deserializing FDSP messages
*
* @tparam PayloadT - FDSP payload type
* @param payloadBuf - payload buffer
* @param payload - return deserialized payload
*
* @return
*/
template<class PayloadT>
void deserializeFdspMsg(const std::string& payloadBuf, PayloadT& payload) {
    // TODO(Rao): Do buffer managment so that the below deserialization is
    // efficient
    bo::shared_ptr<tt::TMemoryBuffer> memory_buf(
        new tt::TMemoryBuffer(reinterpret_cast<uint8_t*>(
                const_cast<char*>(payloadBuf.data())), payloadBuf.size()));
    bo::shared_ptr<tp::TProtocol> binary_buf(new tp::TBinaryProtocol(memory_buf));

    try {
        auto read = payload.read(binary_buf.get());
        fds_verify(read > 0);
    } catch(std::exception &e) {
        /* This is to ensure we assert on any serialization exceptions in debug
         * builds.  We then rethrow the exception
         */
        GLOGDEBUG << "Excpetion in deserializing: " << e.what();
        DBG(std::exception_ptr eptr = std::current_exception());
        fds_assert(!"Exception deserializing.  Most likely due to fdsp msg id mismatch");
        throw;
    } catch(...) {
        /* This is to ensure we assert on any serialization exceptions in debug
         * builds.  We then rethrow the exception
         */
        DBG(std::exception_ptr eptr = std::current_exception());
        fds_assert(!"Exception deserializing.  Most likely due to fdsp msg id mismatch");
        throw;
    }
}

template<class PayloadT>
void deserializeFdspMsg(const bo::shared_ptr<std::string> &payloadBuf,
                        bo::shared_ptr<PayloadT>& payload) {
    if (!payloadBuf) {
        return;
    }

    if (!payload) {
        payload = bo::make_shared<PayloadT>();
    }
    deserializeFdspMsg(*payloadBuf, *payload);
}

template<class PayloadT> boost::shared_ptr<PayloadT>
deserializeFdspMsg(Error &e, boost::shared_ptr<std::string> payloadBuf)
{
    DBG(GLOGTRACE);

    if (e != ERR_OK) {
        return nullptr;
    }
    try {
        boost::shared_ptr<PayloadT> payload(boost::make_shared<PayloadT>());
        deserializeFdspMsg(payloadBuf, payload);
        return payload;
    } catch(std::exception& ex) {
        GLOGWARN << "Failed to deserialize. Exception: " << ex.what();
        e = ERR_SERIALIZE_FAILED;
        return nullptr;
    }
}

}  // namespace fds
#endif  // SOURCE_INCLUDE_FDSP_UTILS_H_
