/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_FDSP_UTILS_H_
#define SOURCE_INCLUDE_FDSP_UTILS_H_

#include <string>
#include <fds_types.h>
#include <fdsp/FDSP_types.h>
#include <persistent_layer/dm_metadata.h>

/* Forward declarations */
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

namespace fds {
/* Forward declarations */
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
}  // namespace fds
#endif  // SOURCE_INCLUDE_FDSP_UTILS_H_
