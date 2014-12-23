/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_PLAT_WORK_ITEM_H_
#define SOURCE_PLATFORM_INCLUDE_PLAT_WORK_ITEM_H_

#include "platform/node_work_item.h"

namespace fds
{
    class PlatWorkItem : public NodeWorkItem
    {
        public:
            typedef boost::intrusive_ptr<PlatWorkItem> ptr;
            typedef boost::intrusive_ptr<const PlatWorkItem> const_ptr;

            PlatWorkItem(fpi::SvcUuid &peer, fpi::DomainID &did,
                         boost::intrusive_ptr<PmAgent> owner, FsmTable::pointer tab,
                         NodeWorkFlow *mod) : NodeWorkItem(peer, did, owner, tab, mod)
            {
            }

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
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_PLAT_WORK_ITEM_H_
