/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <endpoint-test.h>
#include <util/Log.h>

namespace fds {

ProbeEpTestAM                 gl_ProbeTestAM("Probe AM EP");
ProbeEpSvcTestAM              gl_ProbeSvcTestAM("Probe Svc AM");

class AMClntPlugin : public EpEvtPlugin
{
  public:
    void ep_connected()
    {
        fpi::SvcUuid             uuid;
        fpi::ProbeAmCreatVol     cmd;
        fpi::ProbeAmCreatVolResp ret;
        auto rpc = ep_handle->svc_rpc<fpi::ProbeServiceAMClient>();

        ep_handle->ep_peer_uuid(uuid);
        std::cout << "Connected, send test cmd " << uuid.svc_uuid << std::endl;
        rpc->am_creat_vol(ret, cmd);
    }
    void ep_down() {}
    void svc_up(EpSvcHandle::pointer hd) {}
    void svc_down(EpSvc::pointer svc, EpSvcHandle::pointer hd) {}
};

/*
 * -----------------------------------------------------------------------------------
 *  ProbeEpTestAM
 * -----------------------------------------------------------------------------------
 */
int
ProbeEpTestAM::mod_init(SysParams const *const p)
{
    return Module::mod_init(p);
}

void
ProbeEpTestAM::mod_startup()
{
    Module::mod_startup();
}

void
ProbeEpTestAM::mod_enable_service()
{
}

void
ProbeEpTestAM::mod_shutdown()
{
}

/*
 * -----------------------------------------------------------------------------------
 *  ProbeEpSvcTestAM
 * -----------------------------------------------------------------------------------
 */
int
ProbeEpSvcTestAM::mod_init(SysParams const *const p)
{
    Module::mod_init(p);
    return 0;
}

void
ProbeEpSvcTestAM::mod_startup()
{
}

void
ProbeEpSvcTestAM::mod_shutdown()
{
}

}  // namespace fds
