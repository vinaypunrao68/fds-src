/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

/*
#include <string>
*/

#include <platform/platform-lib.h>

namespace fds
{
    // -------------------------------------------------------------------------------------
    // Platform Utility Functions.
    // -------------------------------------------------------------------------------------
    /* static */ int Platform::plf_get_my_node_svc_uuid(fpi::SvcUuid *uuid,
                                                        fpi::FDSP_MgrIdType type)
    {
        NodeUuid    svc_uuid;
        Platform   *plat = Platform::platf_singleton();

        Platform::plf_svc_uuid_from_node(plat->plf_my_uuid, &svc_uuid, type);
        svc_uuid.uuid_assign(uuid);

        return Platform::plf_svc_port_from_node(plat->plf_my_node_port, type);
    }
}  // namespace fds
