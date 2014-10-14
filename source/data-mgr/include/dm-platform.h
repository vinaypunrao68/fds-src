/*
 * Copyright 2014 By Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_PLATFORM_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_PLATFORM_H_

#include <platform/platform-lib.h>

#include <net/net-service.h>
#include <fds_typedefs.h>

/* Forward declarations */
namespace FDS_ProtocolInterface {
class DMSvcClient;
class DMSvcProcessor;
}

namespace fds {

/* Forward declarations */
class DMSvcHandler;
class DmPlatform;

/* DM specific flags */
DBG(DECLARE_FLAG(dm_drop_cat_queries));
DBG(DECLARE_FLAG(dm_drop_cat_updates));

/**
 * This class provides plugin for the endpoint run by DmPlatform
 */
class DMEpPlugin: public EpEvtPlugin
{
  public:
    typedef bo::intrusive_ptr<DMEpPlugin> pointer;
    typedef bo::intrusive_ptr<const DMEpPlugin> const_ptr;

    explicit DMEpPlugin(DmPlatform *dm_plat);
    virtual ~DMEpPlugin();

    void ep_connected();
    void ep_down();

    void svc_up(EpSvcHandle::pointer handle);
    void svc_down(EpSvc::pointer svc, EpSvcHandle::pointer handle);

  protected:
    DmPlatform *dm_plat_;
};

class DmPlatform : public Platform
{
  public:
    DmPlatform();
    virtual ~DmPlatform();

    virtual void plf_reg_node_info(const NodeUuid &uuid, const FdspNodeRegPtr msg);

    /**
     * Module methods
     */
    virtual int  mod_init(SysParams const *const param) override;
    virtual void mod_startup() override;
    virtual void mod_enable_service() override;
    virtual void mod_shutdown() override;

    virtual boost::shared_ptr<BaseAsyncSvcHandler> getBaseAsyncSvcHandler() override;
    inline bo::shared_ptr<DMSvcHandler> getDmRecv() { return dm_recv; }

  protected:
    void registerFlags();

    DMEpPlugin::pointer           dm_plugin;
    bo::shared_ptr<DMSvcHandler>  dm_recv;
    bo::intrusive_ptr<EndPoint<fpi::DMSvcClient, fpi::DMSvcProcessor>> dm_ep;
};

extern DmPlatform gl_DmPlatform;

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_PLATFORM_H_
