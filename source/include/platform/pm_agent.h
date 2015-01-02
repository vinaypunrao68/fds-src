/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_PM_AGENT_H_
#define SOURCE_INCLUDE_PLATFORM_PM_AGENT_H_

#include "node_agent.h"

namespace fds
{
    class PmSvcEp;

    /*
     * -------------------------------------------------------------------------------------
     * Specific node agent type setup for peer to peer communication.
     * -------------------------------------------------------------------------------------
     */
    class PmAgent : public NodeAgent
    {
        public:
            typedef boost::intrusive_ptr<PmAgent> pointer;
            typedef boost::intrusive_ptr<const PmAgent> const_ptr;

            virtual ~PmAgent();
            virtual void agent_bind_ep();
            virtual void agent_svc_info(fpi::NodeSvcInfo *out) const;

            explicit PmAgent(const NodeUuid &uuid);
            boost::intrusive_ptr<PmSvcEp> agent_ep_svc();

        protected:
            boost::intrusive_ptr<PmSvcEp>    pm_ep_svc;

            virtual bo::intrusive_ptr<EpEvtPlugin> agent_ep_plugin();
            virtual void
            agent_svc_fillin(fpi::NodeSvcInfo *,
                             const struct node_data *, fpi::FDSP_MgrIdType) const;
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_PM_AGENT_H_
