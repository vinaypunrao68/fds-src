/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_DM_CONTAINER_H_
#define SOURCE_INCLUDE_PLATFORM_DM_CONTAINER_H_

#include "agent_container.h"

namespace fds
{
    class DmContainer : public AgentContainer
    {
        public:
            typedef boost::intrusive_ptr<DmContainer> pointer;

            explicit DmContainer(FdspNodeType id) : AgentContainer(id)
            {
                ac_id = fpi::FDSP_DATA_MGR;
            }

        protected:
            virtual ~DmContainer()
            {
            }

            virtual Resource *rs_new(const ResourceUUID &uuid)
            {
                return new DmAgent(uuid);
            }
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_DM_CONTAINER_H_
