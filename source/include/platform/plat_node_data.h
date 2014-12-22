/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_PLAT_NODE_DATA_H_
#define SOURCE_INCLUDE_PLATFORM_PLAT_NODE_DATA_H_

#include "shared/fds_types.h"

namespace fds
{
    /**
     * On disk format: POD type to persist the node's inventory.
     */
    typedef struct plat_node_data
    {
        fds_uint32_t nd_chksum;
        fds_uint32_t nd_has_data;       /**< true/false if has valid data.  */
        fds_uint32_t nd_magic;
        fds_uint64_t nd_node_uuid;
        fds_uint32_t nd_node_number;
        fds_uint32_t nd_plat_port;
        fds_uint32_t nd_om_port;
        fds_uint32_t nd_flag_run_sm;
        fds_uint32_t nd_flag_run_dm;
        fds_uint32_t nd_flag_run_am;
        fds_uint32_t nd_flag_run_om;
    } plat_node_data_t;

}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_PLAT_NODE_DATA_H_
