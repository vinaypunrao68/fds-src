/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_ORCH_MGR_NODE_WORKFLOW_H_
#define SOURCE_INCLUDE_ORCH_MGR_NODE_WORKFLOW_H_

#include <ostream>
#include <fds-fsm.h>
#include <fdsp/fds_service_types.h>

namespace fds {

class NodeAgent;
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
    NodeWorkItem(bo::intrusive_ptr<NodeAgent> node, FsmTable::pointer tab);

    /* Generic action functions. */
    virtual void act_node_down();
    virtual void act_node_started();
    virtual void act_node_qualify();
    virtual void act_node_upgrade();
    virtual void act_node_rollback();
    virtual void act_node_integrate();
    virtual void act_node_deploy();
    virtual void act_node_functional();

    friend std::ostream &operator << (std::ostream &os, const NodeWorkItem::pointer);

  protected:
    bo::intrusive_ptr<NodeAgent>   wrk_na;
};

typedef struct node_wrk_flow node_wrk_flow_t;
struct node_wrk_flow
{
    int                          wrk_cur_item;
    fpi::FDSPMsgTypeId           wrk_msg_in;
    int                          wrk_nxt_item;
};

class NodeWorkFlow : public FsmTable
{
  public:
    virtual ~NodeWorkFlow();
    NodeWorkFlow(bo::intrusive_ptr<DomainClusterMap> domain);

    int wrk_next_step(fpi::FDSPMsgTypeId  input);

  protected:
    bo::intrusive_ptr<DomainClusterMap>     wrk_domain;
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_ORCH_MGR_NODE_WORKFLOW_H_
