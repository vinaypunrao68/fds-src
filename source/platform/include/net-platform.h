/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_PLATFORM_INCLUDE_NET_PLATFORM_H_
#define SOURCE_PLATFORM_INCLUDE_NET_PLATFORM_H_

#include <string>
#include <vector>
#include <ep-map.h>
#include <net/PlatNetSvcHandler.h>
#include <platform/net-plat-shared.h>
#include <platform/service-ep-lib.h>

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
    virtual void nplat_register_node(fpi::NodeInfoMsg *, NodeAgent::pointer) override;
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

  protected:
    virtual void agent_publish_ep();
    virtual void agent_bind_svc(EpPlatformdMod *, node_data_t *, fpi::FDSP_MgrIdType);
};

class PlatAgentPlugin : public NodeAgentEvt
{
  public:
    typedef boost::intrusive_ptr<PlatAgentPlugin> pointer;
    typedef boost::intrusive_ptr<const PlatAgentPlugin> const_ptr;

    virtual ~PlatAgentPlugin() {}
    explicit PlatAgentPlugin(NodeAgent::pointer agt) : NodeAgentEvt(agt) {}

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
class PlatformEpHandler : public PlatNetSvcHandler
{
  public:
    virtual ~PlatformEpHandler();
    explicit PlatformEpHandler(PlatformdNetSvc *svc);

    // PlatNetSvcIf methods.
    //
    void allUuidBinding(bo::shared_ptr<fpi::UuidBindMsg> &msg);
    void notifyNodeActive(bo::shared_ptr<fpi::FDSP_ActivateNodeType> &info);

    void notifyNodeInfo(std::vector<fpi::NodeInfoMsg>    &ret,
                        bo::shared_ptr<fpi::NodeInfoMsg> &info,
                        bo::shared_ptr<bool>             &bcast);
    void getDomainNodes(fpi::DomainNodes &ret, fpi::DomainNodesPtr &req);

    virtual void
    NotifyDLTUpdate(boost::shared_ptr<fpi::AsyncHdr>            &hdr,
                    boost::shared_ptr<fpi::CtrlNotifyDLTUpdate> &msg);

    virtual void
    NotifyDMTUpdate(boost::shared_ptr<fpi::AsyncHdr>            &hdr,
                    boost::shared_ptr<fpi::CtrlNotifyDMTUpdate> &msg);

  protected:
    PlatformdNetSvc         *net_plat;
};

}  // namespace fds
#endif  // SOURCE_PLATFORM_INCLUDE_NET_PLATFORM_H_
