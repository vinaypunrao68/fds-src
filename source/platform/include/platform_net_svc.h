/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_PLATFORM_NET_SVC_H_
#define SOURCE_PLATFORM_INCLUDE_PLATFORM_NET_SVC_H_

#include <ep-map.h>

#include <platform/net-plat-shared.h>
#include "platform/net_plat_svc.h"

#include "platform_plugin.h"

namespace fds
{
    /**
     * Net service functions done by platform daemon.  This code produces the lib linked
     * with platform daemon.
     */

    class PlatformEpHandler;

    class PlatformdNetSvc : public NetPlatSvc
    {
        public:
            explicit PlatformdNetSvc(const char *name);
            virtual ~PlatformdNetSvc();

            // Module methods
            ///
            virtual int  mod_init(SysParams const *const p) override;
            virtual void mod_startup() override;
            virtual void mod_enable_service() override;
            virtual void mod_lockstep_start_service() override;
            virtual void mod_shutdown() override;

            // Net platform services
            //
            virtual void nplat_refresh_shm() override;
            virtual void nplat_register_node(fpi::NodeInfoMsg *, NodeAgent::pointer) override;
            virtual void nplat_register_node(const fpi::NodeInfoMsg *msg) override;
            virtual void plat_update_local_binding(const struct ep_map_rec *rec);

            EpSvcHandle::pointer nplat_peer(const fpi::SvcUuid &uuid);
            EpSvcHandle::pointer nplat_peer(const fpi::DomainID &id, const fpi::SvcUuid &uuid);

            inline EpPlatformdMod *plat_rw_shm()
            {
                return plat_shm;
            }

            bo::shared_ptr<PlatNetSvcHandler> getPlatNetSvcHandler();

        protected:
            EpPlatformdMod                      *plat_shm;
            PlatformdPlugin::pointer             plat_plugin;
            bo::shared_ptr<PlatformEpHandler>    plat_recv;
    };

    extern PlatformdNetSvc    gl_PlatformdNetSvc;

}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_PLATFORM_NET_SVC_H_
