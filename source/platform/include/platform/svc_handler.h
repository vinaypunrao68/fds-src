/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_PLATFORM_SVCHANDLER_H_
#define SOURCE_PLATFORM_INCLUDE_PLATFORM_SVCHANDLER_H_

#include <net/PlatNetSvcHandler.h>

#include <fdsp/pm_service_types.h>
#include "fdsp/node_svc_api_types.h"
#include <fdsp/health_monitoring_api_types.h>

namespace fds
{
    namespace pm
    {
        class PlatformManager;

        class SvcHandler : public PlatNetSvcHandler
        {
            public:
                SvcHandler(CommonModuleProviderIf *provider, PlatformManager *platform);

/* to be deprecated START */
                void activateServices(boost::shared_ptr <fpi::AsyncHdr> &hdr, boost::shared_ptr <fpi::ActivateServicesMsg> &activateMsg);
                void deactivateServices(boost::shared_ptr <fpi::AsyncHdr> &hdr, boost::shared_ptr <fpi::DeactivateServicesMsg> &activateMsg);
/* to be deprecated End */

                void addService (boost::shared_ptr <fpi::AsyncHdr> &hdr, boost::shared_ptr <fpi::NotifyAddServiceMsg> &addServiceMessage);
                void removeService (boost::shared_ptr <fpi::AsyncHdr> &hdr, boost::shared_ptr <fpi::NotifyRemoveServiceMsg> &removeServiceMessage);
                void startService (boost::shared_ptr <fpi::AsyncHdr> &hdr, boost::shared_ptr <fpi::NotifyStartServiceMsg> &startServiceMessage);
                void stopService (boost::shared_ptr <fpi::AsyncHdr> &hdr, boost::shared_ptr <fpi::NotifyStopServiceMsg> &stopServiceMessage);

                void heartbeatCheck(boost::shared_ptr <fpi::AsyncHdr>& hdr, boost::shared_ptr<fpi::HeartbeatMessage> &heartbeatMessage);
            protected:
                PlatformManager   *platform;

        };
    }  // namespace pm
}  // namespace fds

#endif // SOURCE_PLATFORM_INCLUDE_PLATFORM_SVCHANDLER_H_
