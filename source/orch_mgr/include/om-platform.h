/*
 * Copyright 2014 By Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OM_PLATFORM_H_
#define SOURCE_ORCH_MGR_INCLUDE_OM_PLATFORM_H_

#include <orchMgr.h>
#include <platform/platform-lib.h>
#include <fds_typedefs.h>
#include <net/net-service.h>

/* Forward declarations */
namespace FDS_ProtocolInterface {
class OMSvcClient;
class OMSvcProcessor;
}

namespace fds {

/* Forward declarations */
class OMSvcHandler;
class OmPlatform;
template <class SendIf, class RecvIf>class EndPoint;

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

typedef EndPoint<fpi::FDSP_ConfigPathReqIf,
                 fpi::FDSP_ConfigPathReqProcessor>      OmConfigEp;
typedef EndPoint<fpi::FDSP_OMControlPathReqIf,
                 fpi::FDSP_OMControlPathReqProcessor>   OmControlEp;
typedef EndPoint<fpi::OMSvcClient, fpi::OMSvcProcessor> OmPlatSvcEp;

typedef bo::intrusive_ptr<OmConfigEp>                   OmConfigEpPtr;
typedef bo::intrusive_ptr<OmControlEp>                  OmControlEpPtr;
typedef bo::intrusive_ptr<OmPlatSvcEp>                  OmPlatSvcEpPtr;

class OmPlatform : public Platform
{
  public:
    OmPlatform();
    virtual ~OmPlatform();

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

    OMEpPlugin::pointer  om_plugin;
    OmConfigEpPtr        om_cfg_ep;
    OmControlEpPtr       om_ctrl_ep;
    OmPlatSvcEpPtr       om_plat_ep;

    bo::shared_ptr<FDSP_ConfigPathReqHandler>     om_cfg_rcv;
    bo::shared_ptr<FDSP_OMControlPathReqHandler>  om_ctrl_rcv;
    bo::shared_ptr<OMSvcHandler>                  om_plat_rcv;
};

extern OmPlatform gl_OmPlatform;

}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_OM_PLATFORM_H_
