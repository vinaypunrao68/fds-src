/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_PLATFORM_INCLUDE_PLAT_NODE_WORKFLOW_H_
#define SOURCE_PLATFORM_INCLUDE_PLAT_NODE_WORKFLOW_H_

#include <platform/node-workflow.h>

namespace fds {

class PlatWorkItem : public NodeWorkItem
{
  public:
    typedef bo::intrusive_ptr<PlatWorkItem> ptr;
    typedef bo::intrusive_ptr<const PlatWorkItem> const_ptr;

    PlatWorkItem(fpi::SvcUuid &peer, fpi::DomainID &did,
                 bo::intrusive_ptr<PmAgent> owner,
                 FsmTable::pointer tab, NodeWorkFlow *mod)
        : NodeWorkItem(peer, did, owner, tab, mod) {}

    /*
     * Generic action functions; provided by derrived class.
     */
    virtual void act_node_down(NodeWrkEvent::ptr, fpi::NodeDownPtr &);
    virtual void act_node_started(NodeWrkEvent::ptr, fpi::NodeInfoMsgPtr &);
    virtual void act_node_qualify(NodeWrkEvent::ptr, fpi::NodeQualifyPtr &);
    virtual void act_node_upgrade(NodeWrkEvent::ptr, fpi::NodeUpgradePtr &);
    virtual void act_node_rollback(NodeWrkEvent::ptr, fpi::NodeUpgradePtr &);
    virtual void act_node_integrate(NodeWrkEvent::ptr, fpi::NodeIntegratePtr &);
    virtual void act_node_deploy(NodeWrkEvent::ptr, fpi::NodeDeployPtr &);
    virtual void act_node_functional(NodeWrkEvent::ptr, fpi::NodeFunctionalPtr &);
};

class PlatWorkFlow;
extern PlatWorkFlow          gl_PlatWorkFlow;

class PlatWorkFlow : public NodeWorkFlow
{
  public:
    /* Singleton access. */
    static PlatWorkFlow *nd_workflow_sgt() { return &gl_PlatWorkFlow; }

    /* Module methods. */
    int  mod_init(SysParams const *const param) override;
    void mod_startup() override;
    void mod_enable_service() override;
    void mod_shutdown() override;

  protected:
    /* Factory method. */
    virtual NodeWorkItem::ptr
    wrk_item_alloc(fpi::SvcUuid               &peer,
                   bo::intrusive_ptr<PmAgent>  owner,
                   bo::intrusive_ptr<DomainContainer> domain);
};

}  // namespace fds
#endif  // SOURCE_PLATFORM_INCLUDE_PLAT_NODE_WORKFLOW_H_
