/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_PM_SVC_EP_H_
#define SOURCE_INCLUDE_PLATFORM_PM_SVC_EP_H_

#include "platform/node_agent_evt.h"

namespace fds
{
    /**
     * PM Service EndPoint
     */
    class PmSvcEp : public EpSvc
    {
        public:
            typedef bo::intrusive_ptr<PmSvcEp> pointer;

            virtual ~PmSvcEp();
            PmSvcEp(boost::intrusive_ptr<NodeAgent> agent, fds_uint32_t maj, fds_uint32_t min,
                    NodeAgentEvt::pointer plugin);

            void svc_receive_msg(const fpi::AsyncHdr &msg);
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_PM_SVC_EP_H_
