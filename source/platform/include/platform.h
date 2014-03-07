/*
 * Copyright 2014 By Formation Data Systems, Inc.
 */
#ifndef SOURCE_PLATFORM_INCLUDE_PLATFORM_H_
#define SOURCE_PLATFORM_INCLUDE_PLATFORM_H_

#include <string>
#include <fds_process.h>
#include <kvstore/configdb.h>
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

/**
 * POD type to persist the node's inventory.
 */
typedef struct plat_node_data
{
    fds_uint32_t              node_chksum;
    fds_uint32_t              node_magic;
    fds_uint64_t              node_uuid;
    fds_uint32_t              node_number;
    fds_uint32_t              node_run_sm;
    fds_uint32_t              node_run_dm;
    fds_uint32_t              node_run_am;
    fds_uint32_t              node_run_om;
} plat_pod_data_t;

/**
 * FDS Platform daemon process.
 */
class PlatformProcess : public FdsProcess
{
  public:
    PlatformProcess(int argc, char *argv[], const std::string &cfg, Module **vec);
    virtual void run();
    virtual void setup();

    inline fds_uint32_t plf_sm_base_port() {
        return plf_mgr->plf_get_my_ctrl_port() + 10;
    }
    inline fds_uint32_t plf_dm_base_port() {
        return plf_mgr->plf_get_my_ctrl_port() + 20;
    }
    inline fds_uint32_t plf_am_base_port() {
        return plf_mgr->plf_get_my_ctrl_port() + 30;
    }

  protected:
    Platform                 *plf_mgr;
    kvstore::ConfigDB        *plf_db;
    plat_pod_data_t           plf_node_data;

  private:
    void plf_load_node_data();
    void plf_apply_node_data();
    void plf_start_node_services();
};

}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_PLATFORM_H_
