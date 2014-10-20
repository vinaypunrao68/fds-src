/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <platform/platform-lib.h>
#include <fdsp/fds_service_types.h>
#include <fdsp/PlatNetSvc.h>
#include <net/net-service-tmpl.hpp>
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

fpi::SvcUuid TestUtils::getAnyNonResidentDmSvcuuid(Platform* platform)
{
    fpi::SvcUuid svcUuid;
    fpi::SvcUuid myDmUuid;
    platform->plf_get_my_dm_svc_uuid(&myDmUuid);

    RsArray dmNodes;
    platform->plf_dm_nodes()->rs_container_snapshot(&dmNodes);
    for (auto n : dmNodes) {
        auto uuid = (int64_t)(n->rs_get_uuid().uuid_get_val());
        // Hack to ignore OM SM svc uuid
        /* TODO(Rao/Vy): Fix this */
        if (uuid == 0xcac1) {
            continue;
        }
        if (myDmUuid.svc_uuid != (int64_t)(n->rs_get_uuid().uuid_get_val())) {
            svcUuid.svc_uuid = n->rs_get_uuid().uuid_get_val();
            break;
        }
    }
    return svcUuid;
}


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

bool TestUtils::enableFault(const fpi::SvcUuid &svcUuid, const std::string &faultId)
{
    auto client = NetMgr::ep_mgr_singleton()->client_rpc<fpi::PlatNetSvcClient>(svcUuid, 0, 0);
    if (!client) {
        return false;
    }
    bool ret = client->setFault("enable name=" + faultId);
    return ret;
}

bool TestUtils::disableFault(const fpi::SvcUuid &svcUuid, const std::string &faultId)
{
    auto client = NetMgr::ep_mgr_singleton()->client_rpc<fpi::PlatNetSvcClient>(svcUuid, 0, 0);
    if (!client) {
        return false;
    }
    bool ret = client->setFault("disable name=" + faultId);
    return ret;
}
}  // namespace fds
