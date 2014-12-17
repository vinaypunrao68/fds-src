/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <net/PlatNetSvcHandler.h>
#include "platform/platform-lib.h"

#include "plat_agent_plugin.h"

namespace fds
{
    // ep_connected
    // ------------
    //
    void PlatAgentPlugin::ep_connected()
    {
        NodeUuid                                 uuid;
        Platform                                *plat;
        NetPlatform                             *net_plat;
        fpi::NodeInfoMsg                        *msg;
        NodeAgent::pointer                       pm;
        EpSvcHandle::pointer                     eph;
        std::vector<fpi::NodeInfoMsg>            ret, ignore;
        bo::shared_ptr<fpi::PlatNetSvcClient>    rpc;

        plat = Platform::platf_singleton();
        msg  = new fpi::NodeInfoMsg();
        na_owner->init_plat_info_msg(msg);

        rpc = na_owner->node_svc_rpc(&eph);
        rpc->notifyNodeInfo(ret, *msg, true);

        std::cout << "Got " << ret.size() << " elements back" << std::endl;

        net_plat = NetPlatform::nplat_singleton(); \

        for (auto it = ret.cbegin(); it != ret.cend(); it++)
        {
            net_plat->nplat_register_node(&*it);

            /* Send to the peer node info about myself. */
            uuid.uuid_set_val((*it).node_loc.svc_node.svc_uuid.svc_uuid);
            pm = plat->plf_find_node_agent(uuid);
            fds_assert(pm != NULL);

            rpc = pm->node_svc_rpc(&eph);
            ignore.clear();
            rpc->notifyNodeInfo(ignore, *msg, false);
        }

        /* Notify completion of platform lockstep. */
        NetPlatform::nplat_singleton()->mod_lockstep_done();
    }

    // ep_down
    // -------
    //
    void PlatAgentPlugin::ep_down()
    {
    }

    // svc_up
    // ------
    //
    void PlatAgentPlugin::svc_up(EpSvcHandle::pointer handle)
    {
    }

    // svc_down
    // --------
    //
    void PlatAgentPlugin::svc_down(EpSvc::pointer svc, EpSvcHandle::pointer handle)
    {
    }

}  // namespace fds
