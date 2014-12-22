/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_PLAT_AGENT_PLUGIN_H_
#define SOURCE_PLATFORM_INCLUDE_PLAT_AGENT_PLUGIN_H_

#include <platform/service-ep-lib.h>

#include "platform/node-inventory.h"
#include "platform/node_agent_evt.h"

namespace fds
{
    class PlatAgentPlugin : public NodeAgentEvt
    {
        public:
            typedef boost::intrusive_ptr<PlatAgentPlugin> pointer;
            typedef boost::intrusive_ptr<const PlatAgentPlugin> const_ptr;

            virtual ~PlatAgentPlugin()
            {
            }

            explicit PlatAgentPlugin(NodeAgent::pointer agt) : NodeAgentEvt(agt)
            {
            }

            void ep_connected() override;
            void ep_down() override;
            void svc_up(EpSvcHandle::pointer handle) override;
            void svc_down(EpSvc::pointer svc, EpSvcHandle::pointer handle) override;
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_PLAT_AGENT_PLUGIN_H_
