/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_AM_CONTAINER_H_
#define SOURCE_INCLUDE_PLATFORM_AM_CONTAINER_H_

#include "agent_container.h"
#include "am_agent.h"

namespace fds
{
    class AmContainer : public AgentContainer
    {
        public:
            typedef boost::intrusive_ptr<AmContainer> pointer;

            explicit AmContainer(FdspNodeType id) : AgentContainer(id)
            {
                ac_id = fpi::FDSP_STOR_HVISOR;
            }

        protected:
            virtual ~AmContainer()
            {
            }

            virtual Resource *rs_new(const ResourceUUID &uuid)
            {
                return new AmAgent(uuid);
            }
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_AM_CONTAINER_H_
