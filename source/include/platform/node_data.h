/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_NODE_DATA_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_DATA_H_

#include "shared/fds-constants.h"
#include "platform/typedefs.h"
#include "platform/node_stor_cap.h"

namespace fds
{
    typedef struct node_data
    {
        fds_uint64_t nd_node_uuid;
        fds_uint64_t nd_service_uuid;

        fds_uint32_t nd_base_port;
        char nd_ip_addr[FDS_MAX_IP_STR];
        char nd_auto_name[FDS_MAX_NODE_NAME];
        char nd_assign_name[FDS_MAX_NODE_NAME];

        FdspNodeType nd_svc_type;
        fds_uint32_t nd_svc_mask;
        fds_uint64_t nd_dlt_version;
        fds_uint32_t nd_disk_type;
        node_stor_cap_t nd_capability;
    } node_data_t;

}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_NODE_DATA_H_
