/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <ep-map.h>

namespace fds
{
    /* static  */ EpPlatformdMod    gl_PlatformdShmLib("Platformd Shm Lib");

    // ep_shm_singleton
    // ----------------
    //
    /* static */ EpPlatformdMod *EpPlatformdMod::ep_shm_singleton()
    {
        return &gl_PlatformdShmLib;
    }
}  // namespace fds
