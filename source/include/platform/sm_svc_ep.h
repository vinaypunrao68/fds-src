/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_SM_SVC_EP_H_
#define SOURCE_INCLUDE_PLATFORM_SM_SVC_EP_H_

#include "platform/node_agent_evt.h"

namespace fds
{
    class SmAgent;

    /**
     * SM Service EndPoint
     */
    class SmSvcEp : public EpSvc
    {
        public:
            typedef bo::intrusive_ptr<SmSvcEp> pointer;

            virtual ~SmSvcEp();
            SmSvcEp(boost::intrusive_ptr<NodeAgent> agent, fds_uint32_t maj, fds_uint32_t min,
                    NodeAgentEvt::pointer plugin);

            void svc_receive_msg(const fpi::AsyncHdr &msg);

        protected:
            boost::intrusive_ptr<SmAgent>    na_owner;
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_SM_SVC_EP_H_
