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
}

namespace fds {

FDS_ProtocolInterface::FDS_ObjectIdType&
assign(FDS_ProtocolInterface::FDS_ObjectIdType& lhs, const ObjectID& rhs);

FDS_ProtocolInterface::FDS_ObjectIdType&
assign(FDS_ProtocolInterface::FDS_ObjectIdType& lhs, const meta_obj_id_t& rhs);

std::string logString(const FDS_ProtocolInterface::AsyncHdr &header);

}  // namespace fds
#endif  // SOURCE_INCLUDE_FDSP_UTILS_H_
