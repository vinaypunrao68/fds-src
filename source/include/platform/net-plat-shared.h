/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_NET_SERVICE_INCLUDE_NET_PLAT_SHARED_H_
#define SOURCE_NET_SERVICE_INCLUDE_NET_PLAT_SHARED_H_

#include <string>
#include <vector>
#include <map>
#include <fds-shmobj.h>
#include <net/net-service.h>
#include <fdsp/PlatNetSvc.h>
#include <platform/platform-lib.h>
#include <platform/node-inventory.h>
#include <net/PlatNetSvcHandler.h>

namespace fds {
class Platform;
class NetPlatSvc;
class NetPlatHandler;
class DomainAgent;
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

    virtual ~DomainAgent();
    explicit DomainAgent(const NodeUuid &uuid, bool alloc_plugin = true);

    inline EpSvcHandle::pointer pda_rpc_handle() {
        return agt_domain_ep;
    }
    inline boost::shared_ptr<fpi::PlatNetSvcClient> pda_rpc() {
        return agt_domain_ep->svc_rpc<fpi::PlatNetSvcClient>();
    }
    virtual void pda_connect_domain(const fpi::DomainID &id);

  protected:
    friend class NetPlatSvc;
    friend class PlatformdNetSvc;

    EpSvcHandle::pointer                  agt_domain_ep;

    virtual void pda_register();
};

template <class T>
static inline T *agt_cast_ptr(DomainAgent::pointer agt)
{
    return static_cast<T *>(get_pointer(agt));
}

/**
 * Iterate through node info records in shared memory.
 */
class NodeInfoShmIter : public ShmObjIter
{
  public:
    virtual ~NodeInfoShmIter() {}
    explicit NodeInfoShmIter(bool rw = false);

    virtual bool
    shm_obj_iter_fn(int idx, const void *k, const void *r, size_t ksz, size_t rsz);

  protected:
    int                       it_no_rec;
    int                       it_no_rec_quit;
    bool                      it_shm_rw;
    const ShmObjRO           *it_shm;
    DomainContainer::pointer  it_local;
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

    virtual DomainAgent::pointer nplat_self() override { return plat_self; }
    virtual DomainAgent::pointer nplat_master() override { return plat_master; }

    // Common net platform services.
    //
    virtual void nplat_refresh_shm();
    virtual EpSvcHandle::pointer nplat_domain_rpc(const fpi::DomainID &id) override;
    virtual void nplat_register_node(fpi::NodeInfoMsg *msg, NodeAgent::pointer node);
    virtual void nplat_register_node(const fpi::NodeInfoMsg *msg) override;

    virtual void nplat_set_my_ep(bo::intrusive_ptr<EpSvcImpl> ep) override;
    virtual bo::intrusive_ptr<EpSvcImpl> nplat_my_ep() override;

    inline std::string const *const nplat_domain_master(int *port) {
        *port = plat_lib->plf_get_om_svc_port();
        return plat_lib->plf_get_om_ip();
    }

  protected:
    PlatNetEpPtr                         plat_ep;
    PlatNetPlugin::pointer               plat_ep_plugin;
    bo::shared_ptr<PlatNetSvcHandler>    plat_ep_hdler;
    DomainAgent::pointer                 plat_self;
    DomainAgent::pointer                 plat_master;
};

/**
 * Internal module vector providing network platform services.
 */
extern NetPlatSvc            gl_NetPlatform;

}  // namespace fds
#endif  // SOURCE_NET_SERVICE_INCLUDE_NET_PLAT_SHARED_H_
