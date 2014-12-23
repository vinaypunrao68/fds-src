/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_DOMAIN_AGENT_H_
#define SOURCE_INCLUDE_PLATFORM_DOMAIN_AGENT_H_

#include "pm_agent.h"

namespace fds
{
    /**
     * Node domain master agent.  This is the main interface to the master domain
     * service.
     */
    class DomainAgent : public PmAgent
    {
        public:
            typedef boost::intrusive_ptr<DomainAgent> pointer;
            typedef boost::intrusive_ptr<const DomainAgent> const_ptr;

            virtual ~DomainAgent();
            explicit DomainAgent(const NodeUuid &uuid, bool alloc_plugin = true);

            inline EpSvcHandle::pointer pda_rpc_handle()
            {
                return agt_domain_ep;
            }

            inline boost::shared_ptr<fpi::PlatNetSvcClient> pda_rpc()
            {
                return agt_domain_ep->svc_rpc<fpi::PlatNetSvcClient>();
            }

            virtual void pda_connect_domain(const fpi::DomainID &id);

        protected:
            friend class NetPlatSvc;
            friend class PlatformdNetSvc;

            EpSvcHandle::pointer    agt_domain_ep;

            virtual void pda_register();
    };

    template <class T> static inline T *agt_cast_ptr(DomainAgent::pointer agt)
    {
        return static_cast<T *>(get_pointer(agt));
    }
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_DOMAIN_AGENT_H_
