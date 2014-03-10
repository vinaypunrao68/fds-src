/*
 * Copyright 2014 By Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_SM_PLATFORM_H_
#define SOURCE_STOR_MGR_INCLUDE_SM_PLATFORM_H_

#include <platform/platform-lib.h>

namespace fds {

class SmVolEvent : public VolPlatEvent
{
  public:
    typedef boost::intrusive_ptr<SmVolEvent> pointer;
    typedef boost::intrusive_ptr<const SmVolEvent> const_ptr;

    virtual ~SmVolEvent() {}
    SmVolEvent(DomainResources::pointer   mgr,
               DomainClusterMap::pointer  clus,
               const Platform            *plf) : VolPlatEvent(mgr, clus, plf) {}

    virtual void plat_evt_handler(const FDSP_MsgHdrTypePtr &hdr);
};

class SmPlatform : public Platform
{
  public:
    SmPlatform();
    virtual ~SmPlatform() {}

    /**
     * Module methods
     */
    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

  protected:
    virtual PlatRpcReqt *plat_creat_reqt_disp();
    virtual PlatRpcResp *plat_creat_resp_disp();
};

/**
 * Storage Manager RPC handlers.  Only overwrite what's specific to SM.
 */
class SmRpcReq : public PlatRpcReqt
{
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
