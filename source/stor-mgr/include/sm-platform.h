/*
 * Copyright 2014 By Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_SM_PLATFORM_H_
#define SOURCE_STOR_MGR_INCLUDE_SM_PLATFORM_H_

#include <platform/platform-lib.h>
#include <net/net-service.h>
#include <fds_typedefs.h>

/* Forward declarations */
namespace FDS_ProtocolInterface {
class SMSvcClient;
class SMSvcProcessor;
}

namespace fds {

/* Forward declarations */
class SMSvcHandler;
class SmPlatform;

/* SM specific flags */
DBG(DECLARE_FLAG(sm_drop_gets));
DBG(DECLARE_FLAG(sm_drop_puts));

class SmVolEvent : public VolPlatEvent {
    public:
     typedef boost::intrusive_ptr<SmVolEvent> pointer;
     typedef boost::intrusive_ptr<const SmVolEvent> const_ptr;

     virtual ~SmVolEvent() {}
     SmVolEvent(DomainResources::pointer   mgr,
                DomainClusterMap::pointer  clus,
                const Platform            *plf) : VolPlatEvent(mgr, clus, plf) {}

     virtual void plat_evt_handler(const FDSP_MsgHdrTypePtr &hdr);
};

/**
 * This class provides plugin for the endpoint run by SmPlatform
 */
class SMEpPlugin: public EpEvtPlugin {
    public:
     typedef bo::intrusive_ptr<SMEpPlugin> pointer;
     typedef bo::intrusive_ptr<const SMEpPlugin> const_ptr;

     explicit SMEpPlugin(SmPlatform *sm_plat);
     virtual ~SMEpPlugin();

     void ep_connected();
     void ep_down();

     void svc_up(EpSvcHandle::pointer handle);
     void svc_down(EpSvc::pointer svc, EpSvcHandle::pointer handle);

    protected:
     SmPlatform *sm_plat_;
};

class SmPlatform : public Platform {
    public:
     SmPlatform();
     virtual ~SmPlatform() {}

     virtual void plf_reg_node_info(const NodeUuid &uuid, const FdspNodeRegPtr msg);

     /**
      * Module methods
      */
     virtual int  mod_init(SysParams const *const param) override;
     virtual void mod_startup() override;
     virtual void mod_enable_service() override;
     virtual void mod_shutdown() override;

     virtual boost::shared_ptr<BaseAsyncSvcHandler> getBaseAsyncSvcHandler();

    protected:
     virtual PlatRpcReqt *plat_creat_reqt_disp();
     virtual PlatRpcResp *plat_creat_resp_disp();
     virtual PlatDataPathResp *plat_creat_dpath_resp();

     void registerFlags();

     SMEpPlugin::pointer           sm_plugin;
     bo::shared_ptr<SMSvcHandler>  sm_recv;
     EndPoint<FDS_ProtocolInterface::SMSvcClient, FDS_ProtocolInterface::SMSvcProcessor> *sm_ep;
};

/**
 * Storage Manager RPC handlers.  Only overwrite what's specific to SM.
 */
class SmRpcReq : public PlatRpcReqt {
    public:
     explicit SmRpcReq(const Platform *plf);
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
     virtual ~SmRpcReq();
};

extern SmPlatform gl_SmPlatform;

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_SM_PLATFORM_H_
