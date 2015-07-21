/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_NODE_AGENT_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_AGENT_H_

#include "node_inventory.h"

namespace fds {
// Forward Declarations
class NodeWorkItem;
class EpSvcHandle;
class EPSvcRequest;
class EpSvcImpl;
class EpSvc;
class EpEvtPlugin;

/**
 * --------------------------------------------------------------------------------------
 * Agent interface to communicate with the remote node.  This is the communication
 * end-point to the node.
 *
 * It's normal that the node agent is there but the transport may not be availble.
 * We'll provide methods to establish the transport in the background and error
 * handling model when the transport is broken.
 * --------------------------------------------------------------------------------------
 */
class NodeAgent : public NodeInventory {
  public:
    typedef boost::intrusive_ptr<NodeAgent> pointer;
    typedef boost::intrusive_ptr<const NodeAgent> const_ptr;

    /**
     * Return the storage weight -- currently capacity in GB / 10
     */
    virtual fds_uint64_t node_stor_weight() const;
    virtual void         node_set_weight(fds_uint64_t weight);

    /**
     * Establish/shutdown the communication with the peer.
     */
    virtual void node_agent_up();
    virtual void node_agent_down();

    virtual boost::shared_ptr<fpi::PlatNetSvcClient>
    node_svc_rpc(boost::intrusive_ptr<EpSvcHandle> *eph, int maj = 0, int min = 0);

    friend std::ostream &operator << (std::ostream &os, const NodeAgent::pointer n);

  protected:
    friend class NodeWorkFlow;
    friend class AgentContainer;

    boost::intrusive_ptr<EpSvcHandle>                    nd_eph;
    boost::intrusive_ptr<EpSvcHandle>                    nd_ctrl_eph;
    boost::shared_ptr<fpi::PlatNetSvcClient>             nd_svc_rpc;

    /* Fixme(Vy): put here until we clean up OM class hierachy. */
    boost::intrusive_ptr<NodeWorkItem>                   pm_wrk_item;

    virtual ~NodeAgent();
    explicit NodeAgent(const NodeUuid &uuid);

    virtual void agent_publish_ep();
    void agent_bind_ep(boost::intrusive_ptr<EpSvcImpl>, boost::intrusive_ptr<EpSvc>);

    virtual boost::intrusive_ptr<EpEvtPlugin> agent_ep_plugin();
    virtual void
    agent_svc_fillin(fpi::NodeSvcInfo *,
                     const struct node_data *, fpi::FDSP_MgrIdType) const;
};

/**
 * Down cast a node agent intrusive pointer.
 */
template <class T> static inline T *agt_cast_ptr(NodeAgent::pointer agt)
{
    return static_cast<T *>(get_pointer(agt));
}

template <class T> static inline T *agt_cast_ptr(Resource::pointer rs)
{
    return static_cast<T *>(get_pointer(rs));
}
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_NODE_AGENT_H_
