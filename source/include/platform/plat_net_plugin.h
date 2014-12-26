/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_PLAT_NET_PLUGIN_H_
#define SOURCE_INCLUDE_PLATFORM_PLAT_NET_PLUGIN_H_

#include "net/net-service.h"

namespace fds
{
    class PlatNetPlugin : public EpEvtPlugin
    {
        public:
            typedef boost::intrusive_ptr<PlatNetPlugin> pointer;
            typedef boost::intrusive_ptr<const PlatNetPlugin> const_ptr;

            explicit PlatNetPlugin(NetPlatSvc *svc);
            virtual ~PlatNetPlugin()
            {
            }

            virtual void ep_connected();
            virtual void ep_down();

            virtual void svc_up(EpSvcHandle::pointer handle);
            virtual void svc_down(EpSvc::pointer svc, EpSvcHandle::pointer handle);

        protected:
            NetPlatSvc   *plat_svc;
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_PLAT_NET_PLUGIN_H_
