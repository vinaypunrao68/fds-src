/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_SERVICE_EP_LIB_H_
#define SOURCE_INCLUDE_PLATFORM_SERVICE_EP_LIB_H_

#include <net/net-service.h>
#include <fdsp/fds_service_types.h>

/**
 * Common shared library to provide SM/DM/AM endpoint services.  These services are
 * embeded in NodeAgent::pointerobjects.
 */

namespace fds
{
    class NodeAgent;
    class SmAgent;
    class DmAgent;
    class AmAgent;
    class OmAgent;

    class NodeAgentEvt : public EpEvtPlugin
    {
        public:
            typedef boost::intrusive_ptr<NodeAgentEvt> pointer;

            virtual ~NodeAgentEvt();
            explicit NodeAgentEvt(boost::intrusive_ptr<NodeAgent>);

            virtual void ep_connected();
            virtual void ep_down();

            virtual void svc_up(EpSvcHandle::pointer eph);
            virtual void svc_down(EpSvc::pointer svc, EpSvcHandle::pointer eph);

        protected:
            boost::intrusive_ptr<NodeAgent>    na_owner;
    };

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

    /**
     * OM Service EndPoint.
     */
    class OmNodeAgentEvt : public NodeAgentEvt
    {
        public:
            typedef boost::intrusive_ptr<OmNodeAgentEvt::pointer> pointer;

            virtual ~OmNodeAgentEvt();
            explicit OmNodeAgentEvt(boost::intrusive_ptr<NodeAgent>);

            virtual void ep_connected();
            virtual void ep_down();

            virtual void svc_up(EpSvcHandle::pointer eph);
            virtual void svc_down(EpSvc::pointer svc, EpSvcHandle::pointer eph);
    };

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

#endif  // SOURCE_INCLUDE_PLATFORM_SERVICE_EP_LIB_H_
