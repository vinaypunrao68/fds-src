/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_NODE_INVENTORY_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_INVENTORY_H_

#include <vector>
#include <string>
#include <fds_ptr.h>
#include <fds_error.h>
#include <fds_resource.h>
#include <fds_module.h>
#include <net/SvcRequest.h>
#include <fdsp/FDSP_types.h>
#include <fdsp/fds_service_types.h>

/*
*/

// New includes
#include "typedefs.h"

#include "node_inventory_x.h"
#include "node_agent.h"

#include "pm_agent.h"
#include "dm_agent.h"
#include "sm_agent.h"
#include "om_agent.h"
#include "am_agent.h"

#include "agent_container.h"


#include "am_container.h"
#include "dm_container.h"
#include "om_container.h"
#include "pm_container.h"
#include "sm_container.h"

#include "domain_container.h"

namespace fds
{
    struct node_data;
    struct node_stor_cap;
    class ShmObjRO;

    /* OM has a known UUID. */
    extern const NodeUuid    gl_OmUuid;
    extern const NodeUuid    gl_OmPmUuid;

    const fds_uint32_t       NODE_DO_PROXY_ALL_SVCS =
                             (fpi::NODE_SVC_SM | fpi::NODE_SVC_DM | fpi::NODE_SVC_AM);
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_NODE_INVENTORY_H_
