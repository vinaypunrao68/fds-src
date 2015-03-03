/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_SM_AGENT_H_
#define SOURCE_INCLUDE_PLATFORM_SM_AGENT_H_

#include <string>

#include "node_agent.h"

namespace fds
{
    class SmSvcEp;

    /*
     * -------------------------------------------------------------------------------------
     * Specific node agent type setup for peer to peer communication.
     * -------------------------------------------------------------------------------------
     */
    class SmAgent : public NodeAgent
    {
        public:
            typedef boost::intrusive_ptr<SmAgent> pointer;
            typedef boost::intrusive_ptr<const SmAgent> const_ptr;

            virtual ~SmAgent();
            virtual void agent_bind_ep();

            explicit SmAgent(const NodeUuid &uuid);
            boost::intrusive_ptr<SmSvcEp> agent_ep_svc();

        protected:
            boost::intrusive_ptr<SmSvcEp>    sm_ep_svc;

            virtual bo::intrusive_ptr<EpEvtPlugin> agent_ep_plugin();
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_SM_AGENT_H_
