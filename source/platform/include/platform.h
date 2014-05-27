/*
 * Copyright 2014 By Formation Data Systems, Inc.
 */
#ifndef SOURCE_PLATFORM_INCLUDE_PLATFORM_H_
#define SOURCE_PLATFORM_INCLUDE_PLATFORM_H_

#include <string>
#include <fds_process.h>
#include <kvstore/platformdb.h>
#include <platform/platform-lib.h>
#include <net-plat-shared.h>

namespace fds {
class DiskPlatModule;
class NodePlatformProc;

class NodePlatform : public Platform
{
  public:
    NodePlatform();
    virtual ~NodePlatform() {}

    /**
     * Module methods
     */
    void mod_load_from_config();
    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

    void        plf_start_node_services(const fpi::FDSP_ActivateNodeTypePtr &msg);
    inline void plf_bind_process(NodePlatformProc *ptr) { plf_process = ptr; }

  protected:
    NodePlatformProc    *plf_process;
    DiskPlatModule      *disk_ctrl;

    virtual PlatRpcReqt *plat_creat_reqt_disp();
    virtual PlatRpcResp *plat_creat_resp_disp();
};

/**
 * Platform daemon RPC handlers.  Only overwrite what's specific to Platform.
 */
class PlatformRpcReqt : public PlatRpcReqt
{
  public:
    explicit PlatformRpcReqt(const Platform *plf);
    virtual ~PlatformRpcReqt();

    void NotifyNodeAdd(fpi::FDSP_MsgHdrTypePtr     &msg_hdr,
                       fpi::FDSP_Node_Info_TypePtr &node_info);

    void NotifyNodeRmv(fpi::FDSP_MsgHdrTypePtr     &msg_hdr,
                       fpi::FDSP_Node_Info_TypePtr &node_info);

    void NotifyNodeActive(fpi::FDSP_MsgHdrTypePtr       &hdr,
                          fpi::FDSP_ActivateNodeTypePtr &info);
};

/**
 * Platform daemon RPC response handlers.
 */
class PlatformRpcResp : public PlatRpcResp
{
  public:
    explicit PlatformRpcResp(const Platform *plf);
    virtual ~PlatformRpcResp();

    void RegisterNodeResp(fpi::FDSP_MsgHdrTypePtr       &hdr,
                          fpi::FDSP_RegisterNodeTypePtr &resp);
};

extern NodePlatform gl_NodePlatform;

class NodePlatformProc : public PlatformProcess
{
  public:
    NodePlatformProc(int argc, char **argv, Module **vec);

    void proc_pre_startup() override;
    int  run() override;

    void plf_fill_disk_capacity_pkt(fpi::FDSP_RegisterNodeTypePtr pkt);

  protected:
    friend class NodePlatform;

    void plf_load_node_data();
    void plf_scan_hw_inventory();
    void plf_start_node_services(const fpi::FDSP_ActivateNodeTypePtr &msg);
};

}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_PLATFORM_H_
