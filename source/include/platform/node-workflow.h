/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_ORCH_MGR_NODE_WORKFLOW_H_
#define SOURCE_INCLUDE_ORCH_MGR_NODE_WORKFLOW_H_

#include <ostream>
#include <fds-fsm.h>
#include <fdsp/fds_service_types.h>

namespace fds {

class PmAgent;
class NodeAgent;
class NodeWorkFlow;
class DomainNodeInv;
class DomainClusterMap;

class NodeDown : public StateEntry
{
  public:
    static inline int st_index() { return 0; }
    virtual char const *const st_name() const { return "NodeDown"; }

    NodeDown() : StateEntry(st_index(), -1) {}
    virtual int st_handle(EventObj::pointer evt, StateObj::pointer cur) const override;
};

class NodeStarted : public StateEntry
{
  public:
    static inline int st_index() { return 1; }
    virtual char const *const st_name() const { return "NodeStarted"; }

    NodeStarted() : StateEntry(st_index(), NodeDown::st_index()) {}
    virtual int st_handle(EventObj::pointer evt, StateObj::pointer cur) const override;

    static int
    wrk_next_step(const StateEntry *entry, EventObj::pointer evt, StateObj::pointer cur);
};

class NodeQualify : public StateEntry
{
  public:
    static inline int st_index() { return 2; }
    virtual char const *const st_name() const { return "NodeQualify"; }

    NodeQualify() : StateEntry(st_index(), NodeDown::st_index()) {}
    virtual int st_handle(EventObj::pointer evt, StateObj::pointer cur) const override;
};

class NodeUpgrade : public StateEntry
{
  public:
    static inline int st_index() { return 3; }
    virtual char const *const st_name() const { return "NodeUpgrade"; }

    NodeUpgrade() : StateEntry(st_index(), NodeDown::st_index()) {}
    virtual int st_handle(EventObj::pointer evt, StateObj::pointer cur) const override;
};

class NodeRollback : public StateEntry
{
  public:
    static inline int st_index() { return 4; }
    virtual char const *const st_name() const { return "NodeRollback"; }

    NodeRollback() : StateEntry(st_index(), NodeDown::st_index()) {}
    virtual int st_handle(EventObj::pointer evt, StateObj::pointer cur) const override;
};

class NodeIntegrate : public StateEntry
{
  public:
    static inline int st_index() { return 5; }
    virtual char const *const st_name() const { return "NodeIntegrate"; }

    NodeIntegrate() : StateEntry(st_index(), NodeDown::st_index()) {}
    virtual int st_handle(EventObj::pointer evt, StateObj::pointer cur) const override;
};

class NodeDeploy : public StateEntry
{
  public:
    static inline int st_index() { return 6; }
    virtual char const *const st_name() const { return "NodeDeploy"; }

    NodeDeploy() : StateEntry(st_index(), NodeDown::st_index()) {}
    virtual int st_handle(EventObj::pointer evt, StateObj::pointer cur) const override;
};

class NodeFunctional : public StateEntry
{
  public:
    static inline int st_index() { return 7; }
    virtual char const *const st_name() const { return "NodeFunctional"; }

    NodeFunctional() : StateEntry(st_index(), NodeDown::st_index()) {}
    virtual int st_handle(EventObj::pointer evt, StateObj::pointer cur) const override;
};

class NodeWorkItem : public StateObj
{
  public:
    typedef bo::intrusive_ptr<NodeWorkItem> ptr;
    typedef bo::intrusive_ptr<const NodeWorkItem> const_ptr;

    virtual ~NodeWorkItem();
    NodeWorkItem(fpi::SvcUuid &peer, fpi::DomainID &did,
                 bo::intrusive_ptr<PmAgent> owner,
                 FsmTable::pointer tab, NodeWorkFlow *mod);

    /* Generic action functions; provided by derrived class. */
    virtual void act_node_down(bo::shared_ptr<fpi::NodeDown> msg);
    virtual void act_node_started(bo::shared_ptr<fpi::NodeInfoMsg> msg);
    virtual void act_node_qualify(bo::shared_ptr<fpi::NodeQualify> msg);
    virtual void act_node_upgrade(bo::shared_ptr<fpi::NodeUpgrade> msg);
    virtual void act_node_rollback(bo::shared_ptr<fpi::NodeUpgrade> msg);
    virtual void act_node_integrate(bo::shared_ptr<fpi::NodeIntegrate> msg);
    virtual void act_node_deploy(bo::shared_ptr<fpi::NodeDeploy> msg);
    virtual void act_node_functional(bo::shared_ptr<fpi::NodeFunctional> msg);

    /* Send data to the peer to notify the peer, provided by the framework. */
    virtual void wrk_send_node_info(bo::shared_ptr<fpi::NodeInfoMsg> msg);
    virtual void wrk_send_node_qualify(bo::shared_ptr<fpi::NodeQualify> msg);
    virtual void wrk_send_node_upgrade(bo::shared_ptr<fpi::NodeUpgrade> msg);
    virtual void wrk_send_node_integrate(bo::shared_ptr<fpi::NodeIntegrate> msg);
    virtual void wrk_send_node_deploy(bo::shared_ptr<fpi::NodeDeploy> msg);
    virtual void wrk_send_node_functional(bo::shared_ptr<fpi::NodeFunctional> msg);
    virtual void wrk_send_node_down(bo::shared_ptr<fpi::NodeDown> msg);

    /* Common function to return the next step based on cur_st and input. */
    int wrk_next_step(EventObj::pointer evt);

  protected:
    fpi::SvcUuid                   wrk_peer_uuid;
    fpi::DomainID                  wrk_peer_did;
    bo::intrusive_ptr<NodeAgent>   wrk_owner;
    NodeWorkFlow                  *wrk_module;

    friend std::ostream &operator << (std::ostream &os, const NodeWorkItem::ptr);
};

typedef struct node_wrk_flow node_wrk_flow_t;
struct node_wrk_flow
{
    int                            wrk_cur_item;
    fpi::FDSPMsgTypeId             wrk_msg_in;
    int                            wrk_nxt_item;
};

extern NodeWorkFlow          gl_NodeWorkFlow;

class NodeWorkFlow : public Module
{
  public:
    NodeWorkFlow();
    virtual ~NodeWorkFlow();

    /* Singleton access. */
    NodeWorkFlow *nd_workflow_sgt() { return &gl_NodeWorkFlow; }

    /* Module methods. */
    int  mod_init(SysParams const *const param) override;
    void mod_startup() override;
    void mod_enable_service() override;
    void mod_shutdown() override;

    /* Factory method. */
    virtual NodeWorkItem::ptr
    wrk_item_create(fpi::SvcUuid                    &peer,
                    bo::intrusive_ptr<PmAgent>       owner,
                    bo::intrusive_ptr<DomainNodeInv> domain);

    /* Entry to process network messages. */
    void wrk_recv_node_info(bo::shared_ptr<fpi::NodeInfoMsg> msg);
    void wrk_recv_node_qualify(bo::shared_ptr<fpi::NodeQualify> msg);
    void wrk_recv_node_upgrade(bo::shared_ptr<fpi::NodeUpgrade> msg);
    void wrk_recv_node_integrate(bo::shared_ptr<fpi::NodeIntegrate> msg);
    void wrk_recv_node_deploy(bo::shared_ptr<fpi::NodeDeploy> msg);
    void wrk_recv_node_functional(bo::shared_ptr<fpi::NodeFunctional> msg);
    void wrk_recv_node_down(bo::shared_ptr<fpi::NodeDown> msg);

  protected:
    FsmTable                               *wrk_fsm;
    bo::intrusive_ptr<DomainClusterMap>     wrk_clus;
    bo::intrusive_ptr<DomainNodeInv>        wrk_inv;

    NodeWorkItem::ptr wrk_item_frm_uuid(fpi::DomainID &did, fpi::SvcUuid &svc, bool inv);
    void wrk_item_submit(fpi::DomainID &, fpi::SvcUuid &, EventObj::pointer);
};

/**
 * Events to change workflow items.
 */
struct NodeWrkEvent : public EventObj
{
    bool                     evt_run_act;

    NodeWrkEvent(int evt_id, bool act) : EventObj(evt_id), evt_run_act(act) {}
};

struct NodeInfoEvt : public NodeWrkEvent
{
    bo::shared_ptr<fpi::NodeInfoMsg>    evt_msg;

    NodeInfoEvt(bo::shared_ptr<fpi::NodeInfoMsg> msg, bool act = true)
        : NodeWrkEvent(fpi::NodeInfoMsgTypeId, act), evt_msg(msg) {}

    virtual void evt_name(std::ostream &os) const { os << "NodeInfo " << evt_msg; }
};

struct NodeQualifyEvt : public NodeWrkEvent
{
    bo::shared_ptr<fpi::NodeQualify>    evt_msg;

    NodeQualifyEvt(bo::shared_ptr<fpi::NodeQualify> msg, bool act = true)
        : NodeWrkEvent(fpi::NodeQualifyTypeId, act), evt_msg(msg) {}

    virtual void evt_name(std::ostream &os) const { os << "NodeQualify " << evt_msg; }
};

struct NodeUpgradeEvt : public NodeWrkEvent
{
    bo::shared_ptr<fpi::NodeUpgrade>    evt_msg;

    NodeUpgradeEvt(bo::shared_ptr<fpi::NodeUpgrade> msg, bool act = true)
        : NodeWrkEvent(fpi::NodeUpgradeTypeId, act), evt_msg(msg) {}

    virtual void evt_name(std::ostream &os) const { os << "NodeUpgrade " << evt_msg; }
};

struct NodeIntegrateEvt : public NodeWrkEvent
{
    bo::shared_ptr<fpi::NodeIntegrate>  evt_msg;

    NodeIntegrateEvt(bo::shared_ptr<fpi::NodeIntegrate> msg, bool act = true)
        : NodeWrkEvent(fpi::NodeIntegrateTypeId, act), evt_msg(msg) {}

    virtual void evt_name(std::ostream &os) const { os << "NodeIntegrate " << evt_msg; }
};

struct NodeDeployEvt : public NodeWrkEvent
{
    bo::shared_ptr<fpi::NodeDeploy>     evt_msg;

    NodeDeployEvt(bo::shared_ptr<fpi::NodeDeploy> msg, bool act = true)
        : NodeWrkEvent(fpi::NodeDeployTypeId, act), evt_msg(msg) {}

    virtual void evt_name(std::ostream &os) const { os << "NodeDeploy " << evt_msg; }
};

struct NodeFunctionalEvt : public NodeWrkEvent
{
    bo::shared_ptr<fpi::NodeFunctional> evt_msg;

    NodeFunctionalEvt(bo::shared_ptr<fpi::NodeFunctional> msg, bool act = true)
        : NodeWrkEvent(fpi::NodeFuncTypeId, act), evt_msg(msg) {}

    virtual void evt_name(std::ostream &os) const { os << "NodeFunct " << evt_msg; }
};

struct NodeDownEvt : public NodeWrkEvent
{
    bo::shared_ptr<fpi::NodeDown>    evt_msg;

    NodeDownEvt(bo::shared_ptr<fpi::NodeDown> msg, bool act = true)
        : NodeWrkEvent(fpi::NodeDownTypeId, act), evt_msg(msg) {}

    virtual void evt_name(std::ostream &os) const { os << "NodeDown " << evt_msg; }
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_ORCH_MGR_NODE_WORKFLOW_H_
