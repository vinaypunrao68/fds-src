/*
 * Copyright 2014 By Formation Data Systems, Inc.
 */
#ifndef SOURCE_PLATFORM_INCLUDE_PLATFORM_H_
#define SOURCE_PLATFORM_INCLUDE_PLATFORM_H_

#include <fds_process.h>
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
    virtual PlatRpcReq *plat_creat_rpc_handler();
};

/**
 * Data Manager RPC handlers.  Only overwrite what's specific to DM.
 */
class PlatformRpcReq : public PlatRpcReq
{
  public:
    PlatformRpcReq();
    virtual ~PlatformRpcReq();

    void NotifyNodeAdd(fpi::FDSP_MsgHdrTypePtr     &msg_hdr,
                       fpi::FDSP_Node_Info_TypePtr &node_info);

    void NotifyNodeRmv(fpi::FDSP_MsgHdrTypePtr     &msg_hdr,
                       fpi::FDSP_Node_Info_TypePtr &node_info);
};

extern NodePlatform gl_NodePlatform;

class PlatformProcess : public FdsProcess
{
  public:
    PlatformProcess(int argc, char *argv[]);
    void run();
};

}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_PLATFORM_H_
