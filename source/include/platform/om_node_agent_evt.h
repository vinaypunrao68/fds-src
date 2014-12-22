/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_OM_NODE_AGENT_EVT_H_
#define SOURCE_INCLUDE_PLATFORM_OM_NODE_AGENT_EVT_H_

#include "platform/node_agent_evt.h"
#include "platform/node_agent.h"

namespace fds
{
    /**
     * OM Service EndPoint.
     */
    class OmNodeAgentEvt : public NodeAgentEvt
    {
        public:
            typedef boost::intrusive_ptr<OmNodeAgentEvt::pointer> pointer;

            virtual ~OmNodeAgentEvt();
            explicit OmNodeAgentEvt(boost::intrusive_ptr<NodeAgent>);

            virtual void ep_connected();
            virtual void ep_down();

            virtual void svc_up(EpSvcHandle::pointer eph);
            virtual void svc_down(EpSvc::pointer svc, EpSvcHandle::pointer eph);
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_OM_NODE_AGENT_EVT_H_
