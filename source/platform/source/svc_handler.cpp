/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <platform/svchandler.h>
#include <net/SvcMgr.h>
#include <net/SvcRequestPool.h>
#include <platform/platformmanager.h>

namespace fds
{
    namespace pm
    {

        SvcHandler::SvcHandler(CommonModuleProviderIf *provider, PlatformManager *platform)
            : PlatNetSvcHandler(provider)
        {
            this->platform = platform;
            REGISTER_FDSP_MSG_HANDLER(fpi::ActivateServicesMsg, activateServices);
        }

        void SvcHandler::activateServices(boost::shared_ptr <fpi::AsyncHdr> &hdr,
                                          boost::shared_ptr <fpi::ActivateServicesMsg> &activateMsg)
        {
            platform->activateServices(activateMsg);
            sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::EmptyMsg), fpi::EmptyMsg());
        }

    }  // namespace pm
}  // namespace fds
