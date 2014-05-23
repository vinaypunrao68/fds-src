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

class NodeInfoIter : public ResourceIter
{
  public:
    virtual ~NodeInfoIter() {}
    explicit NodeInfoIter(std::vector<fpi::NodeInfoMsg> &res) : nd_iter_ret(res) {}

    virtual bool rs_iter_fn(Resource::pointer curr)
    {
        fpi::NodeInfoMsg    msg;
        NodeAgent::pointer  node;

        node = agt_cast_ptr<NodeAgent>(curr);
        node->init_node_info_msg(&msg);
        nd_iter_ret.push_back(msg);
        return true;
    }

  protected:
    std::vector<fpi::NodeInfoMsg>        &nd_iter_ret;
};

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
    NodeInfoIter            iter(ret);
    ShmObjRWKeyUint64      *shm;
    NodeAgent::pointer      agent;
    DomainNodeInv::pointer  local;

    NodeInventory::node_info_msg_to_shm(msg.get(), &rec);
    shm = NodeShmRWCtrl::shm_node_rw_inv();
    idx = shm->shm_insert_rec(static_cast<void *>(&rec.nd_node_uuid),
                              static_cast<void *>(&rec), sizeof(rec));

    // Assert for now to debug any problems with leaking...
    fds_verify(idx != -1);
    local = Platform::platf_singleton()->plf_node_inventory();
    local->dc_register_node(shm, &agent, idx, idx);

    // Go through the entire domain to send back inventory info to the new node.
    local->dc_foreach_am(&iter);
    local->dc_foreach_pm(&iter);
    local->dc_foreach_dm(&iter);
    local->dc_foreach_sm(&iter);

    std::cout << "Node notify idx: " << idx
        << "\nDisk iops max......... " << msg->node_stor.disk_iops_max
        << "\nDisk iops min......... " << msg->node_stor.disk_iops_min
        << "\nDisk capacity......... " << msg->node_stor.disk_capacity
        << "Sent back " << iter.rs_iter_count() << std::endl;
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
