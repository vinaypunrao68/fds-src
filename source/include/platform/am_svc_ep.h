/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_AM_SVC_EP_H_
#define SOURCE_INCLUDE_PLATFORM_AM_SVC_EP_H_

#include "platform/node_agent_evt.h"

namespace fds
{
    /**
     * AM Service EndPoint
     */
    class AmSvcEp : public EpSvc
    {
        public:
            typedef bo::intrusive_ptr<AmSvcEp> pointer;

            virtual ~AmSvcEp();
            AmSvcEp(boost::intrusive_ptr<NodeAgent> agent, fds_uint32_t maj, fds_uint32_t min,
                    NodeAgentEvt::pointer plugin);

            void svc_receive_msg(const fpi::AsyncHdr &msg);

        protected:
            boost::intrusive_ptr<AmAgent>    na_owner;
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_AM_SVC_EP_H_
