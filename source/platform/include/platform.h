/*
 * Copyright 2014 By Formation Data Systems, Inc.
 */
#ifndef SOURCE_PLATFORM_INCLUDE_PLATFORM_H_
#define SOURCE_PLATFORM_INCLUDE_PLATFORM_H_

#include <string>
#include <fds_process.h>
#include <kvstore/platformdb.h>
#include <platform/platform-lib.h>

namespace fds {

class NodePlatform : public Platform
{
  public:
    NodePlatform();
    virtual ~NodePlatform() {}

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

    void setup();
    void run();

  protected:
    void plf_load_node_data();

  private:
    void plf_start_node_services();
};

}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_PLATFORM_H_
