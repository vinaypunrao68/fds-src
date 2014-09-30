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
class DomainContainer;
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

/**
 * Common events to work item.
 */
struct NodeWrkEvent : public EventObj
{
    typedef bo::intrusive_ptr<NodeWrkEvent> ptr;

    bool                     evt_run_act;
    fpi::AsyncHdrPtr         evt_hdr;

    NodeWrkEvent(int evt_id, fpi::AsyncHdrPtr hdr, bool act)
        : EventObj(evt_id), evt_run_act(act), evt_hdr(hdr) {}
};

/**
 * Base work item.
 */
class NodeWorkItem : public StateObj
{
  public:
    typedef bo::intrusive_ptr<NodeWorkItem> ptr;
    typedef bo::intrusive_ptr<const NodeWorkItem> const_ptr;

    virtual ~NodeWorkItem();
    NodeWorkItem(fpi::SvcUuid &peer, fpi::DomainID &did,
                 bo::intrusive_ptr<PmAgent> owner,
                 FsmTable::pointer tab, NodeWorkFlow *mod);

    /*
     * Generic action functions; provided by derrived class.
     */
    virtual void act_node_down(NodeWrkEvent::ptr, bo::shared_ptr<fpi::NodeDown>);
    virtual void act_node_started(NodeWrkEvent::ptr, bo::shared_ptr<fpi::NodeInfoMsg>);
    virtual void act_node_qualify(NodeWrkEvent::ptr, bo::shared_ptr<fpi::NodeQualify>);
    virtual void act_node_upgrade(NodeWrkEvent::ptr, bo::shared_ptr<fpi::NodeUpgrade>);
    virtual void act_node_rollback(NodeWrkEvent::ptr, bo::shared_ptr<fpi::NodeUpgrade>);

    virtual void
    act_node_integrate(NodeWrkEvent::ptr, bo::shared_ptr<fpi::NodeIntegrate>);

    virtual void
    act_node_deploy(NodeWrkEvent::ptr, bo::shared_ptr<fpi::NodeDeploy>);

    virtual void
    act_node_functional(NodeWrkEvent::ptr, bo::shared_ptr<fpi::NodeFunctional>);

    /*
     * Send data to the peer to notify the peer, provided by the framework.
     */
    virtual void
    wrk_send_node_info(fpi::AsyncHdrPtr, bo::shared_ptr<fpi::NodeInfoMsg>);

    virtual void
    wrk_send_node_qualify(fpi::AsyncHdrPtr, bo::shared_ptr<fpi::NodeQualify>);

    virtual void
    wrk_send_node_upgrade(fpi::AsyncHdrPtr, bo::shared_ptr<fpi::NodeUpgrade>);

    virtual void
    wrk_send_node_integrate(fpi::AsyncHdrPtr, bo::shared_ptr<fpi::NodeIntegrate>);

    virtual void
    wrk_send_node_deploy(fpi::AsyncHdrPtr, bo::shared_ptr<fpi::NodeDeploy>);

    virtual void
    wrk_send_node_functional(fpi::AsyncHdrPtr, bo::shared_ptr<fpi::NodeFunctional>);

    virtual void
    wrk_send_node_down(fpi::AsyncHdrPtr, bo::shared_ptr<fpi::NodeDown>);

  protected:
    fpi::SvcUuid                   wrk_peer_uuid;
    fpi::DomainID                  wrk_peer_did;
    bo::intrusive_ptr<NodeAgent>   wrk_owner;
    NodeWorkFlow                  *wrk_module;

    friend std::ostream &operator << (std::ostream &os, const NodeWorkItem::ptr);
};

extern NodeWorkFlow          gl_NodeWorkFlow;

class NodeWorkFlow : public Module
{
  public:
    NodeWorkFlow();
    virtual ~NodeWorkFlow();

    /* Singleton access. */
    static NodeWorkFlow *nd_workflow_sgt() { return &gl_NodeWorkFlow; }
    static void wrk_item_assign(bo::intrusive_ptr<PmAgent> owner, NodeWorkItem::ptr wrk);

    /* Module methods. */
    int  mod_init(SysParams const *const param) override;
    void mod_startup() override;
    void mod_enable_service() override;
    void mod_shutdown() override;

    /* Factory method. */
    virtual void
    wrk_item_create(fpi::SvcUuid               &peer,
                    bo::intrusive_ptr<PmAgent>  owner,
                    bo::intrusive_ptr<DomainContainer> domain);

    /* Entry to process network messages. */
    void wrk_recv_node_info(fpi::AsyncHdrPtr, bo::shared_ptr<fpi::NodeInfoMsg>);
    void wrk_recv_node_qualify(fpi::AsyncHdrPtr, bo::shared_ptr<fpi::NodeQualify>);
    void wrk_recv_node_upgrade(fpi::AsyncHdrPtr, bo::shared_ptr<fpi::NodeUpgrade>);
    void wrk_recv_node_integrate(fpi::AsyncHdrPtr, bo::shared_ptr<fpi::NodeIntegrate>);
    void wrk_recv_node_deploy(fpi::AsyncHdrPtr, bo::shared_ptr<fpi::NodeDeploy>);
    void wrk_recv_node_functional(fpi::AsyncHdrPtr, bo::shared_ptr<fpi::NodeFunctional>);
    void wrk_recv_node_down(fpi::AsyncHdrPtr, bo::shared_ptr<fpi::NodeDown>);

    /* Dump state transitions. */
    inline void wrk_dump_steps(std::stringstream *stt) {
        wrk_fsm->st_dump_state_trans(stt);
    }

  protected:
    FsmTable                               *wrk_fsm;
    bo::intrusive_ptr<DomainClusterMap>     wrk_clus;
    bo::intrusive_ptr<DomainContainer>      wrk_inv;

    NodeWorkItem::ptr wrk_item_frm_uuid(fpi::DomainID &did, fpi::SvcUuid &svc, bool inv);
    void wrk_item_submit(fpi::DomainID &, fpi::SvcUuid &, EventObj::pointer);

    virtual NodeWorkItem::ptr
    wrk_item_alloc(fpi::SvcUuid                &peer,
                   bo::intrusive_ptr<PmAgent>  owner,
                   bo::intrusive_ptr<DomainContainer> domain);
};

/**
 * Events to change workflow items.
 */
struct NodeInfoEvt : public NodeWrkEvent
{
    bo::shared_ptr<fpi::NodeInfoMsg>    evt_msg;

    NodeInfoEvt(fpi::AsyncHdrPtr hdr,
                bo::shared_ptr<fpi::NodeInfoMsg> msg, bool act = true)
        : NodeWrkEvent(fpi::NodeInfoMsgTypeId, hdr, act), evt_msg(msg) {}

    virtual void evt_name(std::ostream &os) const { os << " NodeInfo " << evt_msg; }
};

struct NodeQualifyEvt : public NodeWrkEvent
{
    bo::shared_ptr<fpi::NodeQualify>    evt_msg;

    NodeQualifyEvt(fpi::AsyncHdrPtr hdr,
                   bo::shared_ptr<fpi::NodeQualify> msg, bool act = true)
        : NodeWrkEvent(fpi::NodeQualifyTypeId, hdr, act), evt_msg(msg) {}

    virtual void evt_name(std::ostream &os) const { os << " NodeQualify " << evt_msg; }
};

struct NodeUpgradeEvt : public NodeWrkEvent
{
    bo::shared_ptr<fpi::NodeUpgrade>    evt_msg;

    NodeUpgradeEvt(fpi::AsyncHdrPtr hdr,
                   bo::shared_ptr<fpi::NodeUpgrade> msg, bool act = true)
        : NodeWrkEvent(fpi::NodeUpgradeTypeId, hdr, act), evt_msg(msg) {}

    virtual void evt_name(std::ostream &os) const { os << " NodeUpgrade " << evt_msg; }
};

struct NodeIntegrateEvt : public NodeWrkEvent
{
    bo::shared_ptr<fpi::NodeIntegrate>  evt_msg;

    NodeIntegrateEvt(fpi::AsyncHdrPtr hdr,
                     bo::shared_ptr<fpi::NodeIntegrate> msg, bool act = true)
        : NodeWrkEvent(fpi::NodeIntegrateTypeId, hdr, act), evt_msg(msg) {}

    virtual void evt_name(std::ostream &os) const { os << " NodeIntegrate " << evt_msg; }
};

struct NodeDeployEvt : public NodeWrkEvent
{
    bo::shared_ptr<fpi::NodeDeploy>     evt_msg;

    NodeDeployEvt(fpi::AsyncHdrPtr hdr,
                  bo::shared_ptr<fpi::NodeDeploy> msg, bool act = true)
        : NodeWrkEvent(fpi::NodeDeployTypeId, hdr, act), evt_msg(msg) {}

    virtual void evt_name(std::ostream &os) const { os << " NodeDeploy " << evt_msg; }
};

struct NodeFunctionalEvt : public NodeWrkEvent
{
    bo::shared_ptr<fpi::NodeFunctional> evt_msg;

    NodeFunctionalEvt(fpi::AsyncHdrPtr hdr,
                      bo::shared_ptr<fpi::NodeFunctional> msg, bool act = true)
        : NodeWrkEvent(fpi::NodeFuncTypeId, hdr, act), evt_msg(msg) {}

    virtual void evt_name(std::ostream &os) const { os << " NodeFunct " << evt_msg; }
};

struct NodeDownEvt : public NodeWrkEvent
{
    bo::shared_ptr<fpi::NodeDown>    evt_msg;

    NodeDownEvt(fpi::AsyncHdrPtr hdr,
                bo::shared_ptr<fpi::NodeDown> msg, bool act = true)
        : NodeWrkEvent(fpi::NodeDownTypeId, hdr, act), evt_msg(msg) {}

    virtual void evt_name(std::ostream &os) const { os << " NodeDown " << evt_msg; }
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_ORCH_MGR_NODE_WORKFLOW_H_
