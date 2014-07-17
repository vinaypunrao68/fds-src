/*
 * Copyright 2014 By Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_PLATFORM_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_PLATFORM_H_

#include <platform/platform-lib.h>

#include <net/net-service.h>
#include <fds_typedefs.h>

/* Forward declarations */
namespace FDS_ProtocolInterface {
class DMSvcClient;
class DMSvcProcessor;
}

namespace fds {

/* Forward declarations */
class DMSvcHandler;
class DmPlatform;

/* DM specific flags */
DBG(DECLARE_FLAG(dm_drop_cat_queries));
DBG(DECLARE_FLAG(dm_drop_cat_updates));

class DmVolEvent : public VolPlatEvent
{
  public:
    typedef boost::intrusive_ptr<DmVolEvent> pointer;
    typedef boost::intrusive_ptr<const DmVolEvent> const_ptr;

    virtual ~DmVolEvent() {}
    DmVolEvent(DomainResources::pointer   mgr,
               DomainClusterMap::pointer  clus,
               const Platform            *plf) : VolPlatEvent(mgr, clus, plf) {}

    virtual void plat_evt_handler(const FDSP_MsgHdrTypePtr &hdr);
};

/**
 * This class provides plugin for the endpoint run by DmPlatform
 */
class DMEpPlugin: public EpEvtPlugin
{
  public:
    typedef bo::intrusive_ptr<DMEpPlugin> pointer;
    typedef bo::intrusive_ptr<const DMEpPlugin> const_ptr;

    explicit DMEpPlugin(DmPlatform *dm_plat);
    virtual ~DMEpPlugin();

    void ep_connected();
    void ep_down();

    void svc_up(EpSvcHandle::pointer handle);
    void svc_down(EpSvc::pointer svc, EpSvcHandle::pointer handle);

  protected:
    DmPlatform *dm_plat_;
};

class DmPlatform : public Platform
{
  public:
    DmPlatform();
    virtual ~DmPlatform() {}

    virtual void plf_reg_node_info(const NodeUuid &uuid, const FdspNodeRegPtr msg);

    /**
     * Module methods
     */
    virtual int  mod_init(SysParams const *const param) override;
    virtual void mod_startup() override;
    virtual void mod_enable_service() override;
    virtual void mod_shutdown() override;
    bo::shared_ptr<DMSvcHandler>  dm_recv;

  protected:
    virtual PlatRpcReqt *plat_creat_reqt_disp();
    virtual PlatRpcResp *plat_creat_resp_disp();
    virtual PlatDataPathResp *plat_creat_dpath_resp();

    void registerFlags();

    DMEpPlugin::pointer           dm_plugin;
    EndPoint<FDS_ProtocolInterface::DMSvcClient, FDS_ProtocolInterface::DMSvcProcessor> *dm_ep;
};

/**
 * Data Manager RPC handlers.  Only overwrite what's specific to DM.
 */
class DmRpcReq : public PlatRpcReqt
{
  public:
    explicit DmRpcReq(const Platform *plf);
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

    void NotifyDMTUpdate(fpi::FDSP_MsgHdrTypePtr &msg_hdr,
                         fpi::FDSP_DMT_TypePtr   &dmt_info);

    void NotifyStartMigration(fpi::FDSP_MsgHdrTypePtr    &msg_hdr,
                              fpi::FDSP_DLT_Data_TypePtr &dlt_info);

  protected:
    virtual ~DmRpcReq();
};

extern DmPlatform gl_DmPlatform;

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_PLATFORM_H_
