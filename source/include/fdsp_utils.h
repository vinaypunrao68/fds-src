/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_FDSP_UTILS_H_
#define SOURCE_INCLUDE_FDSP_UTILS_H_

#include <fds_types.h>
#include <fdsp/FDSP_types.h>

namespace fds {

FDS_ProtocolInterface::FDS_ObjectIdType&
assign(FDS_ProtocolInterface::FDS_ObjectIdType& lhs, const ObjectID& rhs);

}  // namespace fds
#endif  // SOURCE_INCLUDE_FDSP_UTILS_H_
