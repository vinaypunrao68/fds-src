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
                      boost::shared_ptr<fpi::ProbeAmCreatVol> &cmd) {}

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
};

ProbeEpTestAM                 gl_ProbeTestAM("Probe AM EP");
ProbeEpSvcTestAM              gl_ProbeSvcTestAM("Probe Svc AM");

/*
 * -----------------------------------------------------------------------------------
 *  ProbeEpTestAM
 * -----------------------------------------------------------------------------------
 */
int
ProbeEpTestAM::mod_init(SysParams const *const p)
{
    NetMgr *mgr;

    Module::mod_init(p);

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
    mgr = NetMgr::ep_mgr_singleton();
    mgr->ep_register(probe_ep);
    mgr->ep_register(ret_probe_ep);
    return 0;
}

void
ProbeEpTestAM::mod_startup()
{
    // probe_ep->ep_activate();
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
    svc.svc_uuid = 0x1234;
    mgr->svc_new_handle<fpi::ProbeServiceSMClient>(mine, svc, &am_hello);

    svc.svc_uuid = 0xcafe;
    mgr->svc_new_handle<fpi::ProbeServiceSMClient>(mine, svc, &am_bye);

    svc.svc_uuid = 0xbeef;
    mgr->svc_new_handle<fpi::ProbeServiceSMClient>(mine, svc, &am_poke);
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
