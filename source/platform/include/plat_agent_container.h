/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_PLAT_AGENT_CONTAINER_H_
#define SOURCE_PLATFORM_INCLUDE_PLAT_AGENT_CONTAINER_H_

#include <string>
#include <vector>
#include <ep-map.h>
#include <net/PlatNetSvcHandler.h>
#include <platform/net-plat-shared.h>
#include <platform/service-ep-lib.h>

#include "plat_agent.h"

namespace fds
{
    class PlatAgentContainer : public PmContainer
    {
        public:
            typedef boost::intrusive_ptr<PlatAgentContainer> pointer;
            typedef boost::intrusive_ptr<const PlatAgentContainer> const_ptr;
            PlatAgentContainer() : PmContainer(fpi::FDSP_PLATFORM)
            {
            }

        protected:
            virtual ~PlatAgentContainer()
            {
            }
            virtual Resource *rs_new(const ResourceUUID &uuid) override
            {
                return new PlatAgent(uuid);
            }
    };

}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_PLAT_AGENT_CONTAINER_H_
