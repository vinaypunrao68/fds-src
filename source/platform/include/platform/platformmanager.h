/*
 * Copyright 2014 By Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_PLATFORMMANAGER_H_
#define SOURCE_PLATFORM_INCLUDE_PLATFORMMANAGER_H_

#include <fds_config.hpp>
#include <fds_module.h>
#include <kvstore/platformdb.h>
#include <fdsp/pm_data_types.h>
#include <fds_typedefs.h>

namespace fds {
namespace pm {

class PlatformManager : public Module {
  public:
    PlatformManager();

    /* Overrides from Module */
    virtual int  mod_init(SysParams const *const param) override;
    virtual void mod_startup() {}
    virtual void mod_shutdown() override;

    int  run();

    pid_t startAM();
    pid_t startSM();
    pid_t startDM();

    void determineDiskCapability();
    void activateServices(const fpi::FDSP_ActivateNodeTypePtr &msg);

  protected:
    bool sendNodeCapabilityToOM();

  private:
    FdsConfigAccessor    *conf;
    fpi::FDSP_AnnounceDiskCapability diskCapability;
    kvstore::PlatformDB *db;
    fpi::NodeInfo nodeInfo;
    std::string rootDir;
};
}  // namespace pm
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_PLATFORMMANAGER_H_
