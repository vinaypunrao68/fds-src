/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_DM_SVC_EP_H_
#define SOURCE_INCLUDE_PLATFORM_DM_SVC_EP_H_

#include "platform/node_agent_evt.h"

namespace fds
{
    class DmAgent;
    class NodeAgent;

    /**
     * DM Service EndPoint
     */
    class DmSvcEp : public EpSvc
    {
        public:
            typedef bo::intrusive_ptr<DmSvcEp> pointer;

            virtual ~DmSvcEp();
            DmSvcEp(boost::intrusive_ptr<NodeAgent> agent, fds_uint32_t maj, fds_uint32_t min,
                    NodeAgentEvt::pointer plugin);

            void svc_receive_msg(const fpi::AsyncHdr &msg);

        protected:
            boost::intrusive_ptr<DmAgent>    na_owner;
    };

}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_DM_SVC_EP_H_
