/*
 * Copyright 2014 By Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_PLATFORM_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_PLATFORM_H_

#include <platform/platform-lib.h>

namespace fds {

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

class DmPlatform : public Platform
{
  public:
    DmPlatform();
    virtual ~DmPlatform() {}

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
