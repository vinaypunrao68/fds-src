/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <ep-map.h>

namespace fds {

EpPlatLibMod                 gl_EpPlatLib("EP Plat");

EpPlatLibMod::EpPlatLibMod(const char *name) : Module(name), ep_shm_map(NULL) {}

// mod_init
// --------
//
int
EpPlatLibMod::mod_init(SysParams const *const p)
{
    Module::mod_init(p);

    return 0;
}

// mod_startup
// -----------
//
void
EpPlatLibMod::mod_startup()
{
}

// mod_shutdown
// ------------
//
void
EpPlatLibMod::mod_shutdown()
{
}

// ep_map_record
// -------------
//
int
EpPlatLibMod::ep_map_record(const ep_map_rec_t *rec)
{
    return 0;
}

// ep_unmap_record
// ---------------
//
int
EpPlatLibMod::ep_unmap_record(fds_uint64_t uuid)
{
    return 0;
}

// ep_lookup_rec
// -------------
//
int
EpPlatLibMod::ep_lookup_rec(fds_uint64_t uuid, ep_map_rec_t *out)
{
    return 0;
}

int
EpPlatLibMod::ep_lookup_rec(const char *name, ep_map_rec_t *out)
{
    return 0;
}

}  // namespace fds
