/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_NODE_WORK_ITEM_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_WORK_ITEM_H_

namespace fds
{
    class NodeWorkFlow;

    /**
     * Base work item.
     */
    class NodeWorkItem : public StateObj
    {
        public:
            typedef bo::intrusive_ptr<NodeWorkItem> ptr;
            typedef bo::intrusive_ptr<const NodeWorkItem> const_ptr;

            virtual ~NodeWorkItem();
            NodeWorkItem(fpi::SvcUuid &peer, fpi::DomainID &did, bo::intrusive_ptr<PmAgent> owner,
                         FsmTable::pointer tab, NodeWorkFlow *mod);

            bool wrk_is_in_om();

            inline static bool wrk_is_om_uuid(fpi::SvcUuid &svc)
            {
                return gl_OmUuid == svc;
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

            /*
             * Send data to the peer to notify it, provided by the framework.
             */
            virtual void wrk_fmt_node_info(fpi::NodeInfoMsgPtr &pkt)
            {
                wrk_owner->init_plat_info_msg(pkt.get());
            }

            virtual void wrk_fmt_node_qualify(fpi::NodeQualifyPtr &);
            virtual void wrk_fmt_node_upgrade(fpi::NodeUpgradePtr &);
            virtual void wrk_fmt_node_integrate(fpi::NodeIntegratePtr &);
            virtual void wrk_fmt_node_deploy(fpi::NodeDeployPtr &);
            virtual void wrk_fmt_node_functional(fpi::NodeFunctionalPtr &);
            virtual void wrk_fmt_node_down(fpi::NodeDownPtr &);
            virtual void wrk_fmt_node_wrkitem(fpi::NodeWorkItemPtr &);

            virtual void wrk_send_node_info(fpi::SvcUuid *, fpi::NodeInfoMsgPtr &, bool);
            virtual void wrk_send_node_qualify(fpi::SvcUuid *, fpi::NodeQualifyPtr &, bool);
            virtual void wrk_send_node_upgrade(fpi::SvcUuid *, fpi::NodeUpgradePtr &, bool);
            virtual void wrk_send_node_integrate(fpi::SvcUuid *, fpi::NodeIntegratePtr &, bool);
            virtual void wrk_send_node_deploy(fpi::SvcUuid *, fpi::NodeDeployPtr &, bool);
            virtual void wrk_send_node_functional(fpi::SvcUuid *, fpi::NodeFunctionalPtr &, bool);
            virtual void wrk_send_node_down(fpi::SvcUuid *, fpi::NodeDownPtr &, bool);

            /*
             * Receive handler to process work item status.
             */
            virtual void wrk_send_node_wrkitem(fpi::SvcUuid *, fpi::NodeWorkItemPtr &, bool);
            virtual void wrk_recv_node_wrkitem(fpi::AsyncHdrPtr &, fpi::NodeWorkItemPtr &);

        protected:
            bool                            wrk_sent_ndown;
            fpi::SvcUuid                    wrk_peer_uuid;
            fpi::DomainID                   wrk_peer_did;
            bo::intrusive_ptr<NodeAgent>    wrk_owner;
            NodeWorkFlow                   *wrk_module;

            void wrk_assign_pkt_uuid(fpi::SvcUuid *uuid);
            friend std::ostream &operator << (std::ostream &os, const NodeWorkItem::ptr);
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_NODE_WORK_ITEM_H_
