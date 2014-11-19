/*
 * Copyright 2014 By Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_AM_PLATFORM_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AM_PLATFORM_H_

#include <platform/platform-lib.h>
#include <net/net-service.h>
#include <AMSvcHandler.h>

/* Forward declarations */
namespace FDS_ProtocolInterface {
class BaseAsyncSvcHandler;
class PlatNetSvcClient;
class PlatNetSvcProcessor;
}


namespace fds {

/* Forward declarations */
class AmPlatform;
class PlatNetSvcHandler;

/**
 * This class provides plugin for the endpoint run by SmPlatform
 */
class AMEpPlugin: public EpEvtPlugin
{
  public:
    typedef bo::intrusive_ptr<AMEpPlugin> pointer;
    typedef bo::intrusive_ptr<const AMEpPlugin> const_ptr;

    explicit AMEpPlugin(AmPlatform *am_plat);
    virtual ~AMEpPlugin();

    void ep_connected();
    void ep_down();

    void svc_up(EpSvcHandle::pointer handle);
    void svc_down(EpSvc::pointer svc, EpSvcHandle::pointer handle);

  protected:
    AmPlatform *am_plat_;
};

class AmPlatform : public Platform
{
  public:
    AmPlatform();
    virtual ~AmPlatform();

    /**
     * Module methods
     */
    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup() override;
    virtual void mod_enable_service() override;
    virtual void mod_shutdown() override;

    virtual boost::shared_ptr<BaseAsyncSvcHandler> getBaseAsyncSvcHandler() override;

    void NotifyAddVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                      fpi::FDSP_NotifyVolTypePtr &vol_msg);

    void NotifyRmVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                     fpi::FDSP_NotifyVolTypePtr &vol_msg);

    void NotifyModVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                      fpi::FDSP_NotifyVolTypePtr &vol_msg);

    void AttachVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                   fpi::FDSP_AttachVolTypePtr &vol_msg);

    void DetachVol(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                   fpi::FDSP_AttachVolTypePtr &vol_msg);

    void NotifyNodeAdd(fpi::FDSP_MsgHdrTypePtr     &msg_hdr,
                       fpi::FDSP_Node_Info_TypePtr &node_info);

    void NotifyNodeRmv(fpi::FDSP_MsgHdrTypePtr     &msg_hdr,
                       fpi::FDSP_Node_Info_TypePtr &node_info);

    void NotifyDLTUpdate(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                         fpi::FDSP_DLT_Data_TypePtr &dlt_info);

    void NotifyDMTUpdate(fpi::FDSP_MsgHdrTypePtr &msg_hdr,   // NOLINT
                         fpi::FDSP_DMT_TypePtr   &dmt_info);

    inline void setInstanceId(fds_uint32_t id) {
        instanceId = id;
    }

  protected:
    AMEpPlugin::pointer           am_plugin;
    bo::shared_ptr<AMSvcHandler>  am_recv;
    bo::intrusive_ptr<EndPoint<fpi::AMSvcClient, fpi::AMSvcProcessor>> am_ep;
    /// Instance ID of this AM
    fds_uint32_t instanceId;
};

extern AmPlatform gl_AmPlatform;

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AM_PLATFORM_H_
