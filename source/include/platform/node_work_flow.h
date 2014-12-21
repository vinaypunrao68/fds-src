/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_NODE_WORK_FLOW_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_WORK_FLOW_H_

#include "fds_module.h"

#include "pm_agent.h"
#include "node_work_item.h"

namespace fds
{
    class NodeWorkFlow;
    extern NodeWorkFlow   *gl_NodeWorkFlow;

    class NodeWorkFlow : public Module
    {
        public:
            NodeWorkFlow();
            virtual ~NodeWorkFlow();

            /* Singleton access. */
            static NodeWorkFlow *nd_workflow_sgt()
            {
                return gl_NodeWorkFlow;
            }
            static void wrk_item_assign(boost::intrusive_ptr<PmAgent> owner, NodeWorkItem::ptr wrk);

            /* Module methods. */
            int  mod_init(SysParams const *const param) override;
            void mod_startup() override;
            void mod_enable_service() override;
            void mod_shutdown() override;

            /* Factory method. */
            virtual void
            wrk_item_create(fpi::SvcUuid &peer, boost::intrusive_ptr<PmAgent>  owner,
                            boost::intrusive_ptr<DomainContainer> domain);

            /* Entry to process network messages. */
            virtual void wrk_recv_node_info(fpi::AsyncHdrPtr &, fpi::NodeInfoMsgPtr &);
            virtual void wrk_recv_node_qualify(fpi::AsyncHdrPtr &, fpi::NodeQualifyPtr &);
            virtual void wrk_recv_node_upgrade(fpi::AsyncHdrPtr &, fpi::NodeUpgradePtr &);
            virtual void wrk_recv_node_deploy(fpi::AsyncHdrPtr &, fpi::NodeDeployPtr &);
            virtual void wrk_recv_node_integrate(fpi::AsyncHdrPtr &, fpi::NodeIntegratePtr &);
            virtual void wrk_recv_node_functional(fpi::AsyncHdrPtr &, fpi::NodeFunctionalPtr &);
            virtual void wrk_recv_node_down(fpi::AsyncHdrPtr &, fpi::NodeDownPtr &);
            virtual void wrk_recv_node_wrkitem(fpi::AsyncHdrPtr &, fpi::NodeWorkItemPtr &);

            /* Notify a node is down. */
            void wrk_node_down(fpi::DomainID &dom, fpi::SvcUuid &svc);

            /* Dump state transitions. */
            inline void wrk_dump_steps(std::stringstream *stt) const
            {
                wrk_fsm->st_dump_state_trans(stt);
            }
            inline static fpi::SvcUuid *wrk_om_uuid()
            {
                return &NodeWorkFlow::nd_workflow_sgt()->wrk_om_dest;
            }

        protected:
            FsmTable                                 *wrk_fsm;
            fpi::SvcUuid                              wrk_om_dest;
            boost::intrusive_ptr<DomainClusterMap>    wrk_clus;
            boost::intrusive_ptr<DomainContainer>     wrk_inv;

            NodeWorkItem::ptr wrk_item_frm_uuid(fpi::DomainID &did, fpi::SvcUuid &svc, bool inv);
            void wrk_item_submit(fpi::DomainID &, fpi::AsyncHdrPtr &, EventObj::pointer);

            virtual NodeWorkItem::ptr
            wrk_item_alloc(fpi::SvcUuid                &peer,
                           boost::intrusive_ptr<PmAgent>  owner,
                           boost::intrusive_ptr<DomainContainer> domain);
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_NODE_WORK_FLOW_H_
