/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_PM_OM_SVC_EP_H_
#define SOURCE_INCLUDE_PLATFORM_PM_OM_SVC_EP_H_

namespace fds
{
    class PM_OmSvcEp : public OmSvcEp
    {
        public:
            typedef bo::intrusive_ptr<PM_OmSvcEp> pointer;

            virtual ~PM_OmSvcEp();
            PM_OmSvcEp(boost::intrusive_ptr<NodeAgent> agent, fds_uint32_t maj, fds_uint32_t min,
                       NodeAgentEvt::pointer plugin);

            virtual void svc_chg_uuid(const NodeUuid &uuid);
            virtual void svc_receive_msg(const fpi::AsyncHdr &msg);
            virtual void ep_first_om_message() override;
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_PM_OM_SVC_EP_H_
