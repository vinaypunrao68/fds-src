/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_PLATFORM_EP_HANDLER_H_
#define SOURCE_PLATFORM_INCLUDE_PLATFORM_EP_HANDLER_H_

#include <string>
#include <vector>
#include <ep-map.h>
#include <net/PlatNetSvcHandler.h>
#include <platform/net-plat-shared.h>
#include <platform/service-ep-lib.h>

#include "platform_net_svc.h"

namespace fds
{
    /**
     * This class provides handler for platform RPC daemon.
     */
    class PlatformEpHandler : public PlatNetSvcHandler
    {
        public:
            virtual ~PlatformEpHandler();
            explicit PlatformEpHandler(PlatformdNetSvc *svc);

            // PlatNetSvcIf methods.
            //
            void allUuidBinding(bo::shared_ptr<fpi::UuidBindMsg> &msg);
            void notifyNodeActive(bo::shared_ptr<fpi::FDSP_ActivateNodeType> &info);

            void notifyNodeInfo(std::vector<fpi::NodeInfoMsg>    &ret,
                                bo::shared_ptr<fpi::NodeInfoMsg> &info,
                                bo::shared_ptr<bool>             &bcast);
            void getDomainNodes(fpi::DomainNodes &ret, fpi::DomainNodesPtr &req);

            virtual void NotifyDLTUpdate(boost::shared_ptr<fpi::AsyncHdr>            &hdr,
                                         boost::shared_ptr<fpi::CtrlNotifyDLTUpdate> &msg);

            virtual void NotifyDMTUpdate(boost::shared_ptr<fpi::AsyncHdr>            &hdr,
                                         boost::shared_ptr<fpi::CtrlNotifyDMTUpdate> &msg);

        protected:
            PlatformdNetSvc   *net_plat;
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_PLATFORM_EP_HANDLER_H_
