/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_PLATFORM_INCLUDE_NET_PLATFORM_H_
#define SOURCE_PLATFORM_INCLUDE_NET_PLATFORM_H_

#include <string>
#include <vector>
#include <ep-map.h>
#include <net-plat-shared.h>

namespace fds {

/**
 * Net service functions done by platform daemon.  This code produces the lib linked
 * with platform daemon.
 */
class PlatformdNetSvc;
class PlatformEpHandler;

/**
 * This class provides plugin for the endpoint run by platform daemon to represent a
 * node.
 */
class PlatformdPlugin : public EpEvtPlugin
{
  public:
    typedef bo::intrusive_ptr<PlatformdPlugin> pointer;
    typedef bo::intrusive_ptr<const PlatformdPlugin> const_ptr;

    explicit PlatformdPlugin(PlatformdNetSvc *svc);
    virtual ~PlatformdPlugin();

    void ep_connected();
    void ep_down();

    void svc_up(EpSvcHandle::pointer handle);
    void svc_down(EpSvc::pointer svc, EpSvcHandle::pointer handle);

  protected:
    PlatformdNetSvc         *plat_svc;
};

class PlatformdNetSvc : public NetPlatSvc
{
  public:
    explicit PlatformdNetSvc(const char *name);
    virtual ~PlatformdNetSvc();

    // Module methods
    ///
    virtual int  mod_init(SysParams const *const p) override;
    virtual void mod_startup() override;
    virtual void mod_enable_service() override;
    virtual void mod_shutdown() override;

    // Net platform services
    //
    virtual void nplat_refresh_shm() override;
    virtual void nplat_register_node(const fpi::NodeInfoMsg *msg) override;
    virtual void plat_update_local_binding(const struct ep_map_rec *rec);

    EpSvcHandle::pointer nplat_peer(const fpi::SvcUuid &uuid);
    EpSvcHandle::pointer nplat_peer(const fpi::DomainID &id, const fpi::SvcUuid &uuid);

    inline EpPlatformdMod *plat_rw_shm() { return plat_shm; }

  protected:
    EpPlatformdMod                    *plat_shm;
    PlatformdPlugin::pointer           plat_plugin;
    bo::shared_ptr<PlatformEpHandler>  plat_recv;
};

/**
 * Platform specific node agent and its container.
 */
class PlatAgent : public DomainAgent
{
  public:
    typedef boost::intrusive_ptr<PlatAgent> pointer;
    typedef boost::intrusive_ptr<const PlatAgent> const_ptr;

    virtual ~PlatAgent() {}
    explicit PlatAgent(const NodeUuid &uuid);

    virtual void pda_register();
    virtual void init_stor_cap_msg(fpi::StorCapMsg *msg) const override;
};

class PlatAgentPlugin : public DomainAgentPlugin
{
  public:
    typedef boost::intrusive_ptr<PlatAgentPlugin> pointer;
    typedef boost::intrusive_ptr<const PlatAgentPlugin> const_ptr;

    virtual ~PlatAgentPlugin() {}
    explicit PlatAgentPlugin(PlatAgent::pointer agt) : DomainAgentPlugin(agt) {}

    void ep_connected() override;
    void ep_down() override;
    void svc_up(EpSvcHandle::pointer handle) override;
    void svc_down(EpSvc::pointer svc, EpSvcHandle::pointer handle) override;
};

class PlatAgentContainer : public PmContainer
{
  public:
    typedef boost::intrusive_ptr<PlatAgentContainer> pointer;
    typedef boost::intrusive_ptr<const PlatAgentContainer> const_ptr;
    PlatAgentContainer() : PmContainer(fpi::FDSP_PLATFORM) {}

  protected:
    virtual ~PlatAgentContainer() {}
    virtual Resource *rs_new(const ResourceUUID &uuid) override {
        return new PlatAgent(uuid);
    }
};

/**
 * Module plugin platformd's control vector.
 */
extern PlatformdNetSvc       gl_PlatformdNetSvc;

/**
 * This class provides handler for platform RPC daemon.
 */
class PlatformEpHandler : virtual public fpi::PlatNetSvcIf, public BaseAsyncSvcHandler
{
  public:
    virtual ~PlatformEpHandler();
    explicit PlatformEpHandler(PlatformdNetSvc *svc);

    // PlatNetSvcIf methods.
    //
    void allUuidBinding(const fpi::UuidBindMsg &mine) {}
    void notifyNodeInfo(std::vector<fpi::NodeInfoMsg> &ret,
                        const fpi::NodeInfoMsg        &info,
                        const bool                     bcast) {}
    void notifyNodeUp(fpi::RespHdr &ret, const fpi::NodeInfoMsg &info) {}

    void allUuidBinding(bo::shared_ptr<fpi::UuidBindMsg> &msg);
    void notifyNodeInfo(std::vector<fpi::NodeInfoMsg>    &ret,
                        bo::shared_ptr<fpi::NodeInfoMsg> &info,
                        bo::shared_ptr<bool>             &bcast);
    void notifyNodeUp(fpi::RespHdr &ret, bo::shared_ptr<fpi::NodeInfoMsg> &info);

  protected:
    PlatformdNetSvc         *net_plat;
};

}  // namespace fds
#endif  // SOURCE_PLATFORM_INCLUDE_NET_PLATFORM_H_
