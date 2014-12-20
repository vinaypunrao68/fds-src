/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_OM_AGENT_H_
#define SOURCE_INCLUDE_PLATFORM_OM_AGENT_H_

#include <string>

#include "node_agent.h"

// Forward declarations
namespace FDS_ProtocolInterface
{
    // class FDSP_ControlPathReqClient;
    class FDSP_OMControlPathReqClient;
    class FDSP_OMControlPathRespProcessor;
    class FDSP_OMControlPathRespIf;
}  // namespace FDS_ProtocolInterface

template <class A, class B, class C> class netClientSessionEx;

typedef netClientSessionEx<fpi::FDSP_OMControlPathReqClient,
                           fpi::FDSP_OMControlPathRespProcessor,
                           fpi::FDSP_OMControlPathRespIf> netOMControlPathClientSession;

namespace fds
{
    namespace apis
    {
        class ConfigurationServiceClient;
    }  // namespace apis

    class OmSvcEp;

    /*
     * -------------------------------------------------------------------------------------
     * Specific node agent type setup for peer to peer communication.
     * -------------------------------------------------------------------------------------
     */
    class OmAgent : public NodeAgent
    {
        public:
            typedef boost::intrusive_ptr<OmAgent> pointer;
            typedef boost::intrusive_ptr<const OmAgent> const_ptr;

            virtual ~OmAgent();
            virtual void agent_bind_ep();

            explicit OmAgent(const NodeUuid &uuid);
            boost::intrusive_ptr<OmSvcEp> agent_ep_svc();

            /**
             * Packet format functions.
             */
            void init_msg_hdr(fpi::FDSP_MsgHdrTypePtr msgHdr) const;
            void init_node_reg_pkt(fpi::FDSP_RegisterNodeTypePtr pkt) const;
            void om_register_node(fpi::FDSP_RegisterNodeTypePtr);

            /**
             * TODO(Vy): remove this API and use the net service one.
             */
            virtual void
            om_handshake(boost::shared_ptr<netSessionTbl> net, std::string om_ip,
                         fds_uint32_t om_port);

            boost::shared_ptr<apis::ConfigurationServiceClient> get_om_config_svc();

        protected:
            netOMControlPathClientSession           *om_sess;    /** the rpc session to OM.  */
            NodeAgentCpOmClientPtr                  om_reqt;     /**< handle to send reqt to OM.  */

            std::string                                            om_sess_id;
            boost::intrusive_ptr<OmSvcEp>                          om_ep_svc;
            boost::shared_ptr<apis::ConfigurationServiceClient>    om_cfg_svc;

            virtual bo::intrusive_ptr<EpEvtPlugin> agent_ep_plugin();
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_OM_AGENT_H_
