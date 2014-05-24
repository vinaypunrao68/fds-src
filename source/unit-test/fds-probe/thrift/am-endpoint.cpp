/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <endpoint-test.h>
#include <util/Log.h>

namespace fds {

// RPC Plugin, the base class is generated by Thrift's IDL.
//
class ProbeTestAM_RPC : virtual public fpi::ProbeServiceAMIf,
                        virtual public BaseAsyncSvcHandler
{
  public:
    ProbeTestAM_RPC() {}
    virtual ~ProbeTestAM_RPC() {}

    void msg_async_resp(const fpi::AsyncHdr &org, const fpi::AsyncHdr &resp) {}
    void msg_async_resp(boost::shared_ptr<fpi::AsyncHdr>    &org,
                        boost::shared_ptr<fpi::AsyncHdr> &resp) {}

    void am_creat_vol(fpi::ProbeAmCreatVolResp &ret,
                      const fpi::ProbeAmCreatVol &cmd) {}
    void am_creat_vol(fpi::ProbeAmCreatVolResp &ret,
                      boost::shared_ptr<fpi::ProbeAmCreatVol> &cmd)
    {
        std::cout << "Am create vol is called" << std::endl;
    }

    void am_probe_put_resp(const fpi::ProbeGetMsgResp &resp) {
        GLOGDEBUG;
        auto r = resp;
        NetMgr::ep_mgr_singleton()->ep_send_async_resp(resp.hdr, r);
    }
    void am_probe_put_resp(boost::shared_ptr<fpi::ProbeGetMsgResp> &resp) {
        GLOGDEBUG;
        auto r = *resp;
        NetMgr::ep_mgr_singleton()->ep_send_async_resp(resp->hdr, r);
        std::cout << __FUNCTION__ << __LINE__;
    }

    void foo(const FDS_ProtocolInterface::ProbeFoo& arg)
    {
        GLOGDEBUG;
    }

    void foo(boost::shared_ptr<FDS_ProtocolInterface::ProbeFoo>&)
    {
        GLOGDEBUG;
    }
};

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
    NetMgr *mgr;
    fpi::SvcUuid uuid;

    Module::mod_enable_service();

    // Allocate the endpoint, bound to a physical port.
    //
    boost::shared_ptr<ProbeTestAM_RPC>hdler(new ProbeTestAM_RPC());
    // probe_ep = new EndPoint<fpi::ProbeServiceSMClient, fpi::ProbeServiceAMProcessor>(
    probe_ep = new EndPoint<fpi::ProbeServiceAMClient, fpi::ProbeServiceAMProcessor>(
            9000,                           /* port number         */
            NodeUuid(0xfedcba),             /* my uuid             */
            NodeUuid(0xabcdef),             /* peer uuid           */
            boost::shared_ptr<fpi::ProbeServiceAMProcessor>(
                new fpi::ProbeServiceAMProcessor(hdler)),
            new ProbeEpPlugin());

    ret_probe_ep = new EndPoint<fpi::ProbeServiceAMClient, fpi::ProbeServiceAMProcessor>(
            9001,                           /* port number         */
            NodeUuid(0xabcdef),             /* my uuid             */
            NodeUuid(0xfedcba),             /* peer uuid           */
            boost::shared_ptr<fpi::ProbeServiceAMProcessor>(
                new fpi::ProbeServiceAMProcessor(hdler)),
            new ProbeEpPlugin());

    // Register the endpoint in the local domain.
    probe_ep->ep_register();
    ret_probe_ep->ep_register();
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
    NetMgr       *mgr;
    fpi::SvcUuid  svc, mine;

    Module::mod_init(p);
    mine.svc_uuid = 0;

    // Locate service handle based on uuid; don't care where they are located.
    //
    mgr          = NetMgr::ep_mgr_singleton();
#if 0
    svc.svc_uuid = 0x1234;
    mgr->svc_get_handle<fpi::ProbeServiceSMClient>(mine, svc, &am_hello, 0, 0);

    svc.svc_uuid = 0xcafe;
    mgr->svc_get_handle<fpi::ProbeServiceSMClient>(mine, svc, &am_bye, 0, 0);

    svc.svc_uuid = 0xbeef;
    mgr->svc_get_handle<fpi::ProbeServiceSMClient>(mine, svc, &am_poke, 0, 0);
#endif
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
