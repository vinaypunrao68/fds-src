/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_NODE_STOR_CAP_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_STOR_CAP_H_

namespace fds
{
    typedef struct node_stor_cap
    {
        fds_uint64_t disk_capacity;
        fds_uint32_t node_iops_max;
        fds_uint32_t node_iops_min;
        fds_uint64_t ssd_capacity;
    } node_stor_cap_t;

}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_NODE_STOR_CAP_H_
