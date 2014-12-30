/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include "platform_plugin.h"

namespace fds
{
    /*
     * -----------------------------------------------------------------------------------
     * Endpoint Plugin
     * -----------------------------------------------------------------------------------
     */
    PlatformdPlugin::~PlatformdPlugin()
    {
    }
    PlatformdPlugin::PlatformdPlugin(PlatformdNetSvc *svc) : EpEvtPlugin(), plat_svc(svc)
    {
    }

    // ep_connected
    // ------------
    //
    void PlatformdPlugin::ep_connected()
    {
    }

    // ep_done
    // -------
    //
    void PlatformdPlugin::ep_down()
    {
    }

    // svc_up
    // ------
    //
    void PlatformdPlugin::svc_up(EpSvcHandle::pointer handle)
    {
    }

    // svc_down
    // --------
    //
    void PlatformdPlugin::svc_down(EpSvc::pointer svc, EpSvcHandle::pointer handle)
    {
    }
}  // namespace fds
