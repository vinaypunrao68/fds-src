/*
 * Copyright 2014 By Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_SM_PLATFORM_H_
#define SOURCE_STOR_MGR_INCLUDE_SM_PLATFORM_H_

#include <platform/platform-lib.h>
#include <net/net-service.h>
#include <fds_typedefs.h>

#include "platform/platform.h"

/* Forward declarations */
namespace FDS_ProtocolInterface {
class SMSvcClient;
class SMSvcProcessor;
}

namespace fds {

/* Forward declarations */
class SMSvcHandler;
class SmPlatform;

/* SM specific flags */
DBG(DECLARE_FLAG(sm_drop_gets));
DBG(DECLARE_FLAG(sm_drop_puts));

/**
 * This class provides plugin for the endpoint run by SmPlatform
 */
class SMEpPlugin: public EpEvtPlugin {
    public:
     typedef bo::intrusive_ptr<SMEpPlugin> pointer;
     typedef bo::intrusive_ptr<const SMEpPlugin> const_ptr;

     explicit SMEpPlugin(SmPlatform *sm_plat);
     virtual ~SMEpPlugin();

     void ep_connected();
     void ep_down();

     void svc_up(EpSvcHandle::pointer handle);
     void svc_down(EpSvc::pointer svc, EpSvcHandle::pointer handle);

    protected:
     SmPlatform *sm_plat_;
};

class SmPlatform : public Platform {
    public:
     SmPlatform();
     virtual ~SmPlatform();

     virtual void plf_reg_node_info(const NodeUuid &uuid, const FdspNodeRegPtr msg);

     /**
      * Module methods
      */
     virtual int  mod_init(SysParams const *const param) override;
     virtual void mod_startup() override;
     virtual void mod_enable_service() override;
     virtual void mod_shutdown() override;

     virtual boost::shared_ptr<BaseAsyncSvcHandler> getBaseAsyncSvcHandler();

    protected:
     void registerFlags();

     SMEpPlugin::pointer           sm_plugin;
     bo::shared_ptr<SMSvcHandler>  sm_recv;
     bo::intrusive_ptr<EndPoint<fpi::SMSvcClient, fpi::SMSvcProcessor>> sm_ep;
};

extern SmPlatform gl_SmPlatform;

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_SM_PLATFORM_H_
