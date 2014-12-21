/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_SM_CONTAINER_H_
#define SOURCE_INCLUDE_PLATFORM_SM_CONTAINER_H_

#include "agent_container.h"

namespace fds
{
    class SmContainer : public AgentContainer
    {
        public:
            typedef boost::intrusive_ptr<SmContainer> pointer;

            explicit SmContainer(FdspNodeType id);

            virtual void agent_handshake(boost::shared_ptr<netSessionTbl> net,
                                         NodeAgent::pointer agent);

        protected:
            virtual ~SmContainer()
            {
            }

            virtual Resource *rs_new(const ResourceUUID &uuid)
            {
                return new SmAgent(uuid);
            }
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_SM_CONTAINER_H_
