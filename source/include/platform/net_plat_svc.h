/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_NET_PLAT_SVC_H_
#define SOURCE_INCLUDE_PLATFORM_NET_PLAT_SVC_H_

#include <string>

#include "net/PlatNetSvcHandler.h"

#include "platform/platform.h"

#include "domain_agent.h"
#include "plat_net_ep_ptr.h"
#include "plat_net_plugin.h"

namespace fds
{
    class NetPlatSvc : public NetPlatform
    {
        public:
            explicit NetPlatSvc(const char *name);
            virtual ~NetPlatSvc();

            // Module methods.
            //
            virtual int  mod_init(SysParams const *const p);
            virtual void mod_startup();
            virtual void mod_enable_service();
            virtual void mod_shutdown();

            virtual DomainAgent::pointer nplat_self() override
            {
                return plat_self;
            }

            virtual DomainAgent::pointer nplat_master() override
            {
                return plat_master;
            }

            // Common net platform services.
            //
            virtual void nplat_refresh_shm();
            virtual EpSvcHandle::pointer nplat_domain_rpc(const fpi::DomainID &id) override;
            virtual void nplat_register_node(fpi::NodeInfoMsg *msg, NodeAgent::pointer node);
            virtual void nplat_register_node(const fpi::NodeInfoMsg *msg) override;

            virtual void nplat_set_my_ep(bo::intrusive_ptr<EpSvcImpl> ep) override;
            virtual bo::intrusive_ptr<EpSvcImpl> nplat_my_ep() override;

            inline std::string const *const nplat_domain_master(int *port)
            {
                *port = plat_lib->plf_get_om_svc_port();
                return plat_lib->plf_get_om_ip();
            }

        protected:
            PlatNetEpPtr                         plat_ep;
            PlatNetPlugin::pointer               plat_ep_plugin;
            bo::shared_ptr<PlatNetSvcHandler>    plat_ep_hdler;
            DomainAgent::pointer                 plat_self;
            DomainAgent::pointer                 plat_master;
    };

    /**
     * Internal module vector providing network platform services.
     */
    extern NetPlatSvc    gl_NetPlatform;

}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_NET_PLAT_SVC_H_
