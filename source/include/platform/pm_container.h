/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_PM_CONTAINER_H_
#define SOURCE_INCLUDE_PLATFORM_PM_CONTAINER_H_

#include "agent_container.h"

namespace fds
{
    class PmContainer : public AgentContainer
    {
        public:
            typedef boost::intrusive_ptr<PmContainer> pointer;

            explicit PmContainer(FdspNodeType id) : AgentContainer(id)
            {
                ac_id = fpi::FDSP_PLATFORM;
            }

        protected:
            virtual ~PmContainer()
            {
            }

            virtual Resource *rs_new(const ResourceUUID &uuid)
            {
                return new PmAgent(uuid);
            }
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_PM_CONTAINER_H_
