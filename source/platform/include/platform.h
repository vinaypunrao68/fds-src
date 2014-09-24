/*
 * Copyright 2014 By Formation Data Systems, Inc.
 */
#ifndef SOURCE_PLATFORM_INCLUDE_PLATFORM_H_
#define SOURCE_PLATFORM_INCLUDE_PLATFORM_H_

#include <string>
#include <fds-shmobj.h>
#include <fds_process.h>
#include <kvstore/platformdb.h>
#include <shared/fds-constants.h>
#include <platform/platform-lib.h>
#include <platform/node-inv-shmem.h>

namespace fds {
class ShmObjRWKeyUint64;
class NodeShmRWCtrl;
class DiskPlatModule;
class NodePlatformProc;
struct node_shm_inventory;

class NodePlatform : public Platform
{
  public:
    NodePlatform();
    virtual ~NodePlatform() {}

    /**
     * Module methods
     */
    virtual int  mod_init(SysParams const *const param) override;
    virtual void mod_startup() override;
    virtual void mod_enable_service() override;
    virtual void mod_shutdown() override;

    void        plf_start_node_services(const fpi::FDSP_ActivateNodeTypePtr &msg);
    inline void plf_bind_process(NodePlatformProc *ptr) { plf_process = ptr; }

    virtual boost::intrusive_ptr<PmSvcEp>
    plat_new_pm_svc(NodeAgent::pointer, fds_uint32_t maj, fds_uint32_t min) override;

  protected:
    NodePlatformProc    *plf_process;
    DiskPlatModule      *disk_ctrl;

    virtual void plf_bind_om_node();
};

/**
 * Platform daemon module controls share memory segments.
 */
extern NodeShmRWCtrl         gl_NodeShmRWCtrl;

class NodeShmRWCtrl : public NodeShmCtrl
{
  public:
    virtual ~NodeShmRWCtrl() {}
    explicit NodeShmRWCtrl(const char *name);

    static ShmObjRWKeyUint64 *shm_am_rw_inv() {
        return gl_NodeShmRWCtrl.shm_am_rw;
    }
    static ShmObjRWKeyUint64 *shm_node_rw_inv() {
        return gl_NodeShmRWCtrl.shm_node_rw;
    }
    static ShmObjRW *shm_uuid_rw_binding() {
        return gl_NodeShmRWCtrl.shm_uuid_rw;
    }
    static ShmObjRWKeyUint64 *shm_node_rw_inv(FdspNodeType type)
    {
        if (type == fpi::FDSP_STOR_HVISOR) {
            return gl_NodeShmRWCtrl.shm_am_rw;
        }
        return gl_NodeShmRWCtrl.shm_node_rw;
    }

    /**
     * Module methods
     */
    virtual int  mod_init(SysParams const *const p) override;
    virtual void mod_startup() override;
    virtual void mod_shutdown() override;

  protected:
    ShmObjRWKeyUint64       *shm_am_rw;
    ShmObjRWKeyUint64       *shm_node_rw;
    ShmObjRW                *shm_uuid_rw;

    void shm_init_queue(node_shm_queue_t *queue);
    void shm_init_header(node_shm_inventory_t *hdr);

    virtual void shm_setup_queue() override;
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
