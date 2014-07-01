/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_FDSP_UTILS_H_
#define SOURCE_INCLUDE_FDSP_UTILS_H_

#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <fds_types.h>
#include <fdsp/FDSP_types.h>
#include <persistent_layer/dm_metadata.h>
#include <boost/make_shared.hpp>

/**
 * Maps FDSPMsg type to FDSPMsgTypeId enum
 */
#define FDSP_MSG_TYPEID(FDSPMsgT) FDSPMsgT##TypeId

// Forward declarations
namespace apache { namespace thrift { namespace transport {
    class TSocket;
    class TTransport;
}}}  // namespace apache::thrift::transport

namespace FDS_ProtocolInterface {
    class AsyncHdr;
    class SvcUuid;
    class GetObjectMsg;
    class GetObjectResp;
    class QueryCatalogMsg;
    class PutObjectMsg;
    class PutObjectRspMsg;
    class UpdateCatalogMsg;
    class UpdateCatalogRspMsg;
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

FDS_ProtocolInterface::SvcUuid&
assign(FDS_ProtocolInterface::SvcUuid& lhs, const ResourceUUID& rhs);

std::string logString(const FDS_ProtocolInterface::AsyncHdr &header);
std::string logString(const FDS_ProtocolInterface::GetObjectMsg &getObj);
std::string logString(const FDS_ProtocolInterface::GetObjectResp &getObj);
std::string logString(const FDS_ProtocolInterface::QueryCatalogMsg& qryCat);
std::string logString(const FDS_ProtocolInterface::PutObjectMsg& putObj);
std::string logString(const FDS_ProtocolInterface::PutObjectRspMsg& putObj);
std::string logString(const FDS_ProtocolInterface::UpdateCatalogMsg& updCat);
std::string logString(const FDS_ProtocolInterface::UpdateCatalogRspMsg& updCat);


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
    bo::shared_ptr<tt::TMemoryBuffer> buffer(new tt::TMemoryBuffer());
    bo::shared_ptr<tp::TProtocol> binary_buf(new tp::TBinaryProtocol(buffer));
    auto written = payload.write(binary_buf.get());
    fds_verify(written > 0);
    payloadBuf = bo::make_shared<std::string>();
    *payloadBuf = buffer->getBufferAsString();
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
void deserializeFdspMsg(const bo::shared_ptr<std::string> &payloadBuf,
                               bo::shared_ptr<PayloadT>& payload)
{
    if (!payloadBuf) {
        return;
    }
    // TODO(Rao): Do buffer managment so that the below deserialization is
    // efficient
    bo::shared_ptr<tt::TMemoryBuffer> memory_buf(
        new tt::TMemoryBuffer(reinterpret_cast<uint8_t*>(
                const_cast<char*>(payloadBuf->c_str())), payloadBuf->size()));
    bo::shared_ptr<tp::TProtocol> binary_buf(new tp::TBinaryProtocol(memory_buf));

    payload = bo::make_shared<PayloadT>();
    auto read = payload->read(binary_buf.get());
    fds_verify(read > 0);
}
}  // namespace fds
#endif  // SOURCE_INCLUDE_FDSP_UTILS_H_
