/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <fds_assert.h>
#include <om-discovery.h>
#include <OmResources.h>
#include <kvstore/configdbmodule.h>

namespace fds {

OmDiscoveryMod               gl_OmDiscoveryMod("OM Discovery");

OmDiscoveryMod::~OmDiscoveryMod() {}
OmDiscoveryMod::OmDiscoveryMod(char const *const name)
    : Module(name), om_vol_mgr(NULL) {}

/**
 * Module methods.
 */
int
OmDiscoveryMod::mod_init(SysParams const *const param)
{
    (void)Module::mod_init(param);

    om_vol_mgr = OM_NodeDomainMod::om_loc_domain_ctrl()->om_vol_mgr();

    fds_verify(om_vol_mgr != NULL);
    om_vol_mgr->vol_reg_disc_mgr(this);
    return 0;
}

void
OmDiscoveryMod::mod_startup()
{
    vol_restore();
}

void
OmDiscoveryMod::mod_shutdown()
{
}

}  // namespace fds
