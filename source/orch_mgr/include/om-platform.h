/*
 * Copyright 2014 By Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OM_PLATFORM_H_
#define SOURCE_ORCH_MGR_INCLUDE_OM_PLATFORM_H_

#include <platform/platform-lib.h>

#include <net/net-service.h>
#include <fds_typedefs.h>

/* Forward declarations */
namespace FDS_ProtocolInterface {
class OMSvcClient;
class OMSvcProcessor;
}

namespace fds {

/* Forward declarations */
class OMSvcHandler;
class OmPlatform;

/**
 * This class provides plugin for the endpoint run by OmPlatform
 */
class OMEpPlugin: public EpEvtPlugin
{
  public:
    typedef bo::intrusive_ptr<OMEpPlugin> pointer;
    typedef bo::intrusive_ptr<const OMEpPlugin> const_ptr;

    explicit OMEpPlugin(OmPlatform *om_plat);
    virtual ~OMEpPlugin();

    void ep_connected();
    void ep_down();

    void svc_up(EpSvcHandle::pointer handle);
    void svc_down(EpSvc::pointer svc, EpSvcHandle::pointer handle);

  protected:
    OmPlatform *om_plat_;
};

class OmPlatform : public Platform
{
  public:
    OmPlatform();
    virtual ~OmPlatform() {}

    /**
     * Module methods
     */
    virtual int  mod_init(SysParams const *const param) override;
    virtual void mod_startup() override;
    virtual void mod_enable_service() override;
    virtual void mod_shutdown() override;

  protected:
    virtual PlatRpcReqt *plat_creat_reqt_disp();
    virtual PlatRpcResp *plat_creat_resp_disp();
    virtual PlatDataPathResp *plat_creat_dpath_resp();

    OMEpPlugin::pointer           om_plugin;
    // bo::shared_ptr<OMSvcHandler>  om_recv;
    // EndPoint<FDS_ProtocolInterface::OMSvcClient, FDS_ProtocolInterface::OMSvcProcessor> *om_ep;
};

extern OmPlatform gl_OmPlatform;

}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_OM_PLATFORM_H_
