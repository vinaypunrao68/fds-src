/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_NODE_AGENT_EVT_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_AGENT_EVT_H_

#include "net/net-service.h"

namespace fds
{
    class NodeAgent;

    class NodeAgentEvt : public EpEvtPlugin
    {
        public:
            typedef boost::intrusive_ptr<NodeAgentEvt> pointer;

            virtual ~NodeAgentEvt();
            explicit NodeAgentEvt(boost::intrusive_ptr<NodeAgent>);

            virtual void ep_connected();
            virtual void ep_down();

            virtual void svc_up(EpSvcHandle::pointer eph);
            virtual void svc_down(EpSvc::pointer svc, EpSvcHandle::pointer eph);

        protected:
            boost::intrusive_ptr<NodeAgent>    na_owner;
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_NODE_AGENT_EVT_H_
