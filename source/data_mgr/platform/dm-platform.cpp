/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <dm-platform.h>

namespace fds {

DmPlatform gl_DmPlatform;

// -------------------------------------------------------------------------------------
// DM Platform Event Handler
// -------------------------------------------------------------------------------------
void
DmVolEvent::plat_evt_handler()
{
}

// -------------------------------------------------------------------------------------
// DM Specific Platform
// -------------------------------------------------------------------------------------
DmPlatform::DmPlatform()
    : Platform("DM-Platform",
               new DomainNodeInv("DM-Platform-NodeInv",
                                 NULL,
                                 new SmContainer(FDSP_DATA_MGR),
                                 new DmContainer(FDSP_DATA_MGR),
                                 new AmContainer(FDSP_DATA_MGR),
                                 new PmContainer(FDSP_DATA_MGR),
                                 new OmContainer(FDSP_DATA_MGR)),
               new DomainClusterMap("DM-Platform-ClusMap",
                                 NULL,
                                 new SmContainer(FDSP_DATA_MGR),
                                 new DmContainer(FDSP_DATA_MGR),
                                 new AmContainer(FDSP_DATA_MGR),
                                 new PmContainer(FDSP_DATA_MGR),
                                 new OmContainer(FDSP_DATA_MGR)),
               new DomainResources("DM-Resources"),
               NULL)
{
}

int
DmPlatform::mod_init(SysParams const *const param)
{
    Platform::mod_init(param);

    return 0;
}

void
DmPlatform::mod_startup()
{
}

void
DmPlatform::mod_shutdown()
{
}

}  // namespace fds
