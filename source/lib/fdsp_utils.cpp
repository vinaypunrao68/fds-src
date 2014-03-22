/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <fdsp_utils.h>

namespace fds {

FDS_ProtocolInterface::FDS_ObjectIdType&
assign(FDS_ProtocolInterface::FDS_ObjectIdType& lhs, const ObjectID& rhs)
{
    lhs.digest.assign(reinterpret_cast<const char*>(rhs.GetId()), rhs.GetLen());
    return lhs;
}

}  // namespace fds
