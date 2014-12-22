/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_OM_SVC_EP_H_
#define SOURCE_INCLUDE_PLATFORM_OM_SVC_EP_H_

#include "platform/node_agent_evt.h"

namespace fds
{
    class OmSvcEp : public EpSvc
    {
        public:
            typedef bo::intrusive_ptr<OmSvcEp> pointer;

            virtual ~OmSvcEp();
            OmSvcEp(boost::intrusive_ptr<NodeAgent> agent, fds_uint32_t maj, fds_uint32_t min,
                    NodeAgentEvt::pointer plugin);

            virtual void svc_chg_uuid(const NodeUuid &uuid);
            virtual void svc_receive_msg(const fpi::AsyncHdr &msg);

            virtual void ep_om_connected();
            virtual void ep_domain_connected();
            virtual void ep_first_om_message();

        protected:
            bool                             ep_conn_om;
            bool                             ep_conn_domain;
            boost::intrusive_ptr<OmAgent>    na_owner;
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_OM_SVC_EP_H_
