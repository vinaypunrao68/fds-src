/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OM_DISCOVERY_H_
#define SOURCE_ORCH_MGR_INCLUDE_OM_DISCOVERY_H_

#include <fds_module.h>
#include <OmVolume.h>

namespace fds {

class OmDiscoveryMod : public Module
{
  public:
    virtual ~OmDiscoveryMod();
    explicit OmDiscoveryMod(char const *const name);

    virtual int vol_persist(VolumeInfo::pointer vol);
    virtual void vol_restore();

    /**
     * Module methods.
     */
    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

  protected:
    VolumeContainer::pointer  om_vol_mgr;
};

extern OmDiscoveryMod         gl_OmDiscoveryMod;

}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_OM_DISCOVERY_H_
