/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_PLATFORM_PLUGIN_H_
#define SOURCE_PLATFORM_INCLUDE_PLATFORM_PLUGIN_H_

namespace fds
{
    class PlatformdNetSvc;

    /**
     * This class provides plugin for the endpoint run by platform daemon to represent a
     * node.
     */
    class PlatformdPlugin : public EpEvtPlugin
    {
        public:
            typedef bo::intrusive_ptr<PlatformdPlugin> pointer;
            typedef bo::intrusive_ptr<const PlatformdPlugin> const_ptr;

            explicit PlatformdPlugin(PlatformdNetSvc *svc);
            virtual ~PlatformdPlugin();

            void ep_connected();
            void ep_down();

            void svc_up(EpSvcHandle::pointer handle);
            void svc_down(EpSvc::pointer svc, EpSvcHandle::pointer handle);

        protected:
            PlatformdNetSvc   *plat_svc;
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_PLATFORM_PLUGIN_H_
