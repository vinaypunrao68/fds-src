/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_NET_SERVICE_INCLUDE_NET_PLAT_SHARED_H_
#define SOURCE_NET_SERVICE_INCLUDE_NET_PLAT_SHARED_H_

#include <string>
#include <vector>
#include <net/net-service.h>
#include <fdsp/PlatNetSvc.h>
#include <platform/node-inventory.h>

namespace fds {
class Platform;
class NetPlatSvc;
class NetPlatHandler;
template <class SendIf, class RecvIf>class EndPoint;
struct ep_map_rec;

typedef EndPoint<fpi::PlatNetSvcClient, fpi::PlatNetSvcProcessor>  PlatNetEp;
typedef bo::intrusive_ptr<PlatNetEp>  PlatNetEpPtr;

class PlatNetPlugin : public EpEvtPlugin
{
  public:
    typedef boost::intrusive_ptr<PlatNetPlugin> pointer;
    typedef boost::intrusive_ptr<const PlatNetPlugin> const_ptr;

    explicit PlatNetPlugin(NetPlatSvc *svc);
    virtual ~PlatNetPlugin() {}

    virtual void ep_connected();
    virtual void ep_down();

    virtual void svc_up(EpSvcHandle::pointer handle);
    virtual void svc_down(EpSvc::pointer svc, EpSvcHandle::pointer handle);

  protected:
    NetPlatSvc              *plat_svc;
};

/**
 * Node domain master agent.  This is the main interface to the master domain
 * service.
 */
class DomainAgent : public PmAgent
{
  public:
    typedef boost::intrusive_ptr<DomainAgent> pointer;
    typedef boost::intrusive_ptr<const DomainAgent> const_ptr;

    virtual ~DomainAgent() {}
    explicit DomainAgent(const NodeUuid &uuid) : PmAgent(uuid) {}

    inline EpSvcHandle::pointer pda_rpc()  { return agt_domain_ep; }
    virtual void pda_connect_domain(const fpi::DomainID &id);
    virtual void pda_update_binding(const struct ep_map_rec *rec, int cnt);

  protected:
    EpSvcHandle::pointer                  agt_domain_ep;
    bo::shared_ptr<fpi::PlatNetSvcClient> agt_domain_rpc;
};

class NetPlatSvc : public NetPlatform
{
  public:
    explicit NetPlatSvc(const char *name);
    virtual ~NetPlatSvc();

    // Module methods.
    //
    virtual int  mod_init(SysParams const *const p);
    virtual void mod_startup();
    virtual void mod_enable_service();
    virtual void mod_shutdown();

    EpSvcHandle::pointer nplat_domain_rpc(const fpi::DomainID &id);

  protected:
    PlatNetEpPtr                         plat_ep;
    PlatNetPlugin::pointer               plat_ep_plugin;
    bo::shared_ptr<NetPlatHandler>       plat_ep_hdler;
    DomainAgent::pointer                 plat_agent;
};

/**
 * Internal module vector providing network platform services.
 */
extern NetPlatSvc            gl_NetPlatform;

class NetPlatHandler : virtual public fpi::PlatNetSvcIf
{
  public:
    virtual ~NetPlatHandler() {}
    explicit NetPlatHandler(NetPlatSvc *svc)
        : fpi::BaseAsyncSvcIf(), fpi::PlatNetSvcIf(), net_plat(svc) {}

    // BaseAsyncSvcIf methods.
    //
    void asyncReqt(const fpi::AsyncHdr &hdr) {}
    void asyncResp(const fpi::AsyncHdr &hdr, const std::string &payload) {}
    void uuidBind(fpi::RespHdr &ret, const fpi::UuidBindMsg &msg) {}

    void asyncReqt(bo::shared_ptr<fpi::AsyncHdr> &hdr);
    void asyncResp(bo::shared_ptr<fpi::AsyncHdr> &hdr,
                   bo::shared_ptr<std::string>   &payload);
    void uuidBind(fpi::RespHdr &ret, bo::shared_ptr<fpi::UuidBindMsg> &msg);

    // PlatNetSvcIf methods.
    //
    void allUuidBinding(std::vector<fpi::UuidBindMsg> &ret,
                        const fpi::UuidBindMsg        &mine) {}
    void notifyNodeInfo(std::vector<fpi::NodeInfoMsg> &ret,
                        const fpi::NodeInfoMsg        &info) {}
    void notifyNodeUp(fpi::RespHdr &ret, const fpi::NodeInfoMsg &info) {}

    void allUuidBinding(std::vector<fpi::UuidBindMsg>    &ret,
                        bo::shared_ptr<fpi::UuidBindMsg> &msg);
    void notifyNodeInfo(std::vector<fpi::NodeInfoMsg> &ret,
                        bo::shared_ptr<fpi::NodeInfoMsg> &i);
    void notifyNodeUp(fpi::RespHdr &ret, bo::shared_ptr<fpi::NodeInfoMsg> &info);

  protected:
    NetPlatSvc              *net_plat;
};

}  // namespace fds
#endif  // SOURCE_NET_SERVICE_INCLUDE_NET_PLAT_SHARED_H_
