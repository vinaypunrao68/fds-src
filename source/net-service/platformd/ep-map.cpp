/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <ep-map.h>

namespace fds {

EpPlatformMod                 gl_EpPlatform("EP Platformd");

EpPlatformMod::EpPlatformMod(const char *name) : EpPlatLibMod(name) {}

// mod_init
// --------
//
int
EpPlatformMod::mod_init(SysParams const *const p)
{
    Module::mod_init(p);

    return 0;
}

// mod_startup
// -----------
//
void
EpPlatformMod::mod_startup()
{
}

// mod_shutdown
// ------------
//
void
EpPlatformMod::mod_shutdown()
{
}

// ep_map_record
// -------------
//
int
EpPlatformMod::ep_map_record(const ep_map_rec_t *rec)
{
    return 0;
}

// ep_unmap_record
// ---------------
//
int
EpPlatformMod::ep_unmap_record(fds_uint64_t uuid)
{
    return 0;
}

// ep_lookup_rec
// -------------
//
int
EpPlatformMod::ep_lookup_rec(fds_uint64_t uuid, ep_map_rec_t *out)
{
    return 0;
}

int
EpPlatformMod::ep_lookup_rec(const char *name, ep_map_rec_t *out)
{
    return 0;
}

}  // namespace fds
