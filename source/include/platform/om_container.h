/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_OM_CONTAINER_H_
#define SOURCE_INCLUDE_PLATFORM_OM_CONTAINER_H_

#include "agent_container.h"
#include "om_agent.h"

namespace fds
{
    class OmContainer : public AgentContainer
    {
        public:
            typedef boost::intrusive_ptr<OmContainer> pointer;

            explicit OmContainer(FdspNodeType id) : AgentContainer(id)
            {
                ac_id = fpi::FDSP_ORCH_MGR;
            }

        protected:
            virtual ~OmContainer()
            {
            }

            virtual Resource *rs_new(const ResourceUUID &uuid)
            {
                return new OmAgent(uuid);
            }
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_OM_CONTAINER_H_
