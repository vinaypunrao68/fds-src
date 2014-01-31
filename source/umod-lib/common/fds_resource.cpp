/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <stdlib.h>
#include <fds_resource.h>

namespace fds {

ResourceUUID::ResourceUUID(fds_uint64_t val)
    : rs_uuid(val)
{
}

QueryMgr::QueryMgr()
{
}

QueryMgr::~QueryMgr()
{
}

} // namespace fds
