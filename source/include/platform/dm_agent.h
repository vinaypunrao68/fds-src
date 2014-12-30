/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_DM_AGENT_H_
#define SOURCE_INCLUDE_PLATFORM_DM_AGENT_H_

#include "node_agent.h"

namespace fds
{
    class DmSvcEp;

    /*
     * -------------------------------------------------------------------------------------
     * Specific node agent type setup for peer to peer communication.
     * -------------------------------------------------------------------------------------
     */
    class DmAgent : public NodeAgent
    {
        public:
            typedef boost::intrusive_ptr<DmAgent> pointer;
            typedef boost::intrusive_ptr<const DmAgent> const_ptr;

            virtual ~DmAgent();
            virtual void agent_bind_ep();

            explicit DmAgent(const NodeUuid &uuid);
            boost::intrusive_ptr<DmSvcEp> agent_ep_svc();

        protected:
            boost::intrusive_ptr<DmSvcEp>    dm_ep_svc;

            virtual bo::intrusive_ptr<EpEvtPlugin> agent_ep_plugin();
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_DM_AGENT_H_
