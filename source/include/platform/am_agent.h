/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_AM_AGENT_H_
#define SOURCE_INCLUDE_PLATFORM_AM_AGENT_H_

#include "node_agent.h"

namespace fds
{
    class AmSvcEp;

    /*
     * -------------------------------------------------------------------------------------
     * Specific node agent type setup for peer to peer communication.
     * -------------------------------------------------------------------------------------
     */
    class AmAgent : public NodeAgent
    {
        public:
            typedef boost::intrusive_ptr<AmAgent> pointer;
            typedef boost::intrusive_ptr<const AmAgent> const_ptr;

            virtual ~AmAgent();
            virtual void agent_bind_ep();

            explicit AmAgent(const NodeUuid &uuid);
            boost::intrusive_ptr<AmSvcEp> agent_ep_svc();

        protected:
            boost::intrusive_ptr<AmSvcEp>    am_ep_svc;

            virtual bo::intrusive_ptr<EpEvtPlugin> agent_ep_plugin();
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_AM_AGENT_H_
