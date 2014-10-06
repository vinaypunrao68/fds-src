/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OM_NODE_WORKFLOW_H_
#define SOURCE_ORCH_MGR_INCLUDE_OM_NODE_WORKFLOW_H_

#include <vector>
#include <platform/node-workflow.h>

namespace fds {

class OmWorkItem : public NodeWorkItem
{
  public:
    typedef bo::intrusive_ptr<OmWorkItem> ptr;
    typedef bo::intrusive_ptr<const OmWorkItem> const_ptr;

    OmWorkItem(fpi::SvcUuid &peer, fpi::DomainID &did,
               bo::intrusive_ptr<PmAgent> owner,
               FsmTable::pointer tab, NodeWorkFlow *mod)
        : NodeWorkItem(peer, did, owner, tab, mod) {}

    /*
     * Generic action functions; provided by derrived class.
     */
    virtual void act_node_down(NodeWrkEvent::ptr, fpi::NodeDownPtr &) override;
    virtual void act_node_started(NodeWrkEvent::ptr, fpi::NodeInfoMsgPtr &) override;
    virtual void act_node_qualify(NodeWrkEvent::ptr, fpi::NodeQualifyPtr &) override;
    virtual void act_node_upgrade(NodeWrkEvent::ptr, fpi::NodeUpgradePtr &) override;
    virtual void act_node_rollback(NodeWrkEvent::ptr, fpi::NodeUpgradePtr &) override;
    virtual void act_node_integrate(NodeWrkEvent::ptr, fpi::NodeIntegratePtr &) override;
    virtual void act_node_deploy(NodeWrkEvent::ptr, fpi::NodeDeployPtr &) override;

    virtual void
    act_node_functional(NodeWrkEvent::ptr, fpi::NodeFunctionalPtr &) override;
};

class OmWorkFlow;
extern OmWorkFlow          gl_OmWorkFlow;

class OmWorkFlow : public NodeWorkFlow
{
  public:
    /* Singleton access. */
    static OmWorkFlow *nd_workflow_sgt() { return &gl_OmWorkFlow; }

    /* Module methods. */
    int  mod_init(SysParams const *const param) override;
    void mod_startup() override;
    void mod_enable_service() override;
    void mod_shutdown() override;

    /*
     * Overwrite receive handler.
     */
    virtual void wrk_recv_node_info(fpi::AsyncHdrPtr &, fpi::NodeInfoMsgPtr &) override;

    /* Entry to access to DLT functions. */
    void wrk_comp_dlt_add(bo::intrusive_ptr<NodeAgent>);
    void wrk_comp_dlt_remove(bo::intrusive_ptr<NodeAgent>);
    void wrk_commit_dlt(bo::intrusive_ptr<NodeAgent>, fpi::NodeWorkItemPtr &);
    void wrk_wrkitem_dlt(bo::intrusive_ptr<NodeAgent>, std::vector<fpi::NodeWorkItem> *);

    /* Entry to access to DMT functions. */
    void wrk_comp_dmt_add(bo::intrusive_ptr<NodeAgent>);
    void wrk_comp_dmt_remove(bo::intrusive_ptr<NodeAgent>);
    void wrk_commit_dmt(bo::intrusive_ptr<NodeAgent>, fpi::NodeWorkItemPtr &);
    void wrk_wrkitem_dmt(bo::intrusive_ptr<NodeAgent>, std::vector<fpi::NodeWorkItem> *);

  protected:
    /* Factory method. */
    virtual NodeWorkItem::ptr
    wrk_item_alloc(fpi::SvcUuid               &peer,
                   bo::intrusive_ptr<PmAgent>  owner,
                   bo::intrusive_ptr<DomainContainer> domain);
};

}  // namespace fds
#endif  // SOURCE_ORCH_MGR_INCLUDE_OM_NODE_WORKFLOW_H_
