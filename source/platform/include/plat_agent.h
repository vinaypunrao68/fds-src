/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_PLAT_AGENT_H_
#define SOURCE_PLATFORM_INCLUDE_PLAT_AGENT_H_

#include "ep-map.h"

#include "platform/domain_agent.h"

namespace fds
{
    /**
     * Platform specific node agent and its container.
     */

    class PlatAgent : public DomainAgent
    {
        public:
            typedef boost::intrusive_ptr<PlatAgent> pointer;
            typedef boost::intrusive_ptr<const PlatAgent> const_ptr;

            virtual ~PlatAgent()
            {
            }

            explicit PlatAgent(const NodeUuid &uuid);

            virtual void pda_register();
            virtual void init_stor_cap_msg(fpi::StorCapMsg *msg) const override;

        protected:
            virtual void agent_publish_ep();
            virtual void agent_bind_svc(EpPlatformdMod *, node_data_t *, fpi::FDSP_MgrIdType);
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_PLAT_AGENT_H_
