/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <platform/platform-lib.h>
#include <TestUtils.h>

namespace fds {
/**
* @brief Returns sm svc uuid that is controlled by a different platform daemon than
* the current one
*
* @param platform
*
* @return 
*/
fpi::SvcUuid TestUtils::getAnyNonResidentSmSvcuuid(Platform* platform)
{
    fpi::SvcUuid svcUuid;
    fpi::SvcUuid mySmUuid;
    platform->plf_get_my_sm_svc_uuid(&mySmUuid);

    RsArray smNodes;
    platform->plf_sm_nodes()->rs_container_snapshot(&smNodes);
    for (auto n : smNodes) {
        auto uuid = (int64_t)(n->rs_get_uuid().uuid_get_val());
        // Hack to ignore OM SM svc uuid
        /* TODO(Rao/Vy): Fix this */
        if (uuid == 0xcac1) {
            continue;
        }
        if (mySmUuid.svc_uuid != (int64_t)(n->rs_get_uuid().uuid_get_val())) {
            svcUuid.svc_uuid = n->rs_get_uuid().uuid_get_val();
            break;
        }
    }
    return svcUuid;
}
}  // namespace fds
