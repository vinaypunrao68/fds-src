/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <platform/svc_handler.h>
#include <net/SvcMgr.h>
#include <net/SvcRequestPool.h>
#include <platform/platform_manager.h>

namespace fds
{
    namespace pm
    {
        SvcHandler::SvcHandler(CommonModuleProviderIf *provider, PlatformManager *platform) : PlatNetSvcHandler(provider)
        {
            this->platform = platform;
            REGISTER_FDSP_MSG_HANDLER(fpi::ActivateServicesMsg, activateServices);
            REGISTER_FDSP_MSG_HANDLER(fpi::DeactivateServicesMsg, deactivateServices);
        }

        void SvcHandler::activateServices(boost::shared_ptr <fpi::AsyncHdr> &hdr, boost::shared_ptr <fpi::ActivateServicesMsg> &activateMsg)
        {
            platform->activateServices(activateMsg);
            sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::EmptyMsg), fpi::EmptyMsg());
        }

        void SvcHandler::deactivateServices(boost::shared_ptr <fpi::AsyncHdr> &hdr, boost::shared_ptr <fpi::DeactivateServicesMsg> &deactivateMsg)
        {
            platform->deactivateServices(deactivateMsg);
            sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::EmptyMsg), fpi::EmptyMsg());
        }

    }  // namespace pm
}  // namespace fds
