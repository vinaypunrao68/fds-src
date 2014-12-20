/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_SM_AGENT_H_
#define SOURCE_INCLUDE_PLATFORM_SM_AGENT_H_

#include <string>

#include "node_agent.h"

// Forward declarations
namespace FDS_ProtocolInterface
{
    class FDSP_DataPathReqClient;
    class FDSP_DataPathRespProcessor;
    class FDSP_DataPathRespIf;
}  // namespace FDS_ProtocolInterface

/* TODO(Vy) Need to remove netsession stuffs! */
class netSessionTbl;

template <class A, class B, class C> class netClientSessionEx;

typedef netClientSessionEx<fpi::FDSP_DataPathReqClient,
                           fpi::FDSP_DataPathRespProcessor,
                           fpi::FDSP_DataPathRespIf>      netDataPathClientSession;
namespace fds
{
    class SmSvcEp;

    /*
     * -------------------------------------------------------------------------------------
     * Specific node agent type setup for peer to peer communication.
     * -------------------------------------------------------------------------------------
     */
    class SmAgent : public NodeAgent
    {
        public:
            typedef boost::intrusive_ptr<SmAgent> pointer;
            typedef boost::intrusive_ptr<const SmAgent> const_ptr;

            virtual ~SmAgent();
            virtual void agent_bind_ep();
            virtual void sm_handshake(boost::shared_ptr<netSessionTbl> net);

            explicit SmAgent(const NodeUuid &uuid);
            boost::intrusive_ptr<SmSvcEp> agent_ep_svc();

            NodeAgentDpClientPtr get_sm_client();
            std::string get_sm_sess_id();

        protected:
            netDataPathClientSession        *sm_sess;
            NodeAgentDpClientPtr             sm_reqt;
            std::string                      sm_sess_id;
            boost::intrusive_ptr<SmSvcEp>    sm_ep_svc;

            virtual bo::intrusive_ptr<EpEvtPlugin> agent_ep_plugin();
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_SM_AGENT_H_
