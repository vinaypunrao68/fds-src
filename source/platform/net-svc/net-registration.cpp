/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <vector>
#include <platform.h>
#include <fds-shmobj.h>
#include <net/net-service-tmpl.hpp>
#include <net-platform.h>
#include <platform/node-inv-shmem.h>

namespace fds {

/*
 * -----------------------------------------------------------------------------------
 * RPC Handlers
 * -----------------------------------------------------------------------------------
 */
PlatformEpHandler::~PlatformEpHandler() {}
PlatformEpHandler::PlatformEpHandler(PlatformdNetSvc *svc)
    : fpi::PlatNetSvcIf(), net_plat(svc) {}

// allUuidBinding
// --------------
//
void
PlatformEpHandler::allUuidBinding(std::vector<fpi::UuidBindMsg>    &ret,
                                  bo::shared_ptr<fpi::UuidBindMsg> &msg)
{
    std::cout << "Platform domain master, all uuidBind is called" << std::endl;
}

// notifyNodeInfo
// --------------
//
void
PlatformEpHandler::notifyNodeInfo(std::vector<fpi::NodeInfoMsg>    &ret,
                                  bo::shared_ptr<fpi::NodeInfoMsg> &msg)
{
    int                     idx;
    node_data_t             rec;
    ShmObjRWKeyUint64      *shm;
    NodeAgent::pointer      agent;
    DomainNodeInv::pointer  local;

    NodeInventory::node_info_msg_to_shm(msg, &rec);
    shm = NodeShmRWCtrl::shm_node_rw_inv();
    idx = shm->shm_insert_rec(static_cast<void *>(&rec.nd_node_uuid),
                              static_cast<void *>(&rec), sizeof(rec));

    // Assert for now to debug any problems with leaking...
    fds_verify(idx != -1);
    local = Platform::platf_singleton()->plf_node_inventory();
    local->dc_register_node(shm, &agent, idx, idx);

    std::cout << "Node notify idx: " << idx
        << "\nDisk iops max......... " << msg->node_stor.disk_iops_max
        << "\nDisk iops min......... " << msg->node_stor.disk_iops_min
        << "\nDisk capacity......... " << msg->node_stor.disk_capacity << std::endl;
}

// notifyNodeUp
// ------------
//
void
PlatformEpHandler::notifyNodeUp(fpi::RespHdr &ret,
                                bo::shared_ptr<fpi::NodeInfoMsg> &info)
{
}

}  // namespace fds
