/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <ep-map.h>

namespace fds {

EpPlatformdMod::EpPlatformdMod(const char *name) : EpPlatLibMod(name) {}

// mod_init
// --------
//
int
EpPlatformdMod::mod_init(SysParams const *const p)
{
    Module::mod_init(p);

    return 0;
}

// mod_startup
// -----------
//
void
EpPlatformdMod::mod_startup()
{
}

// mod_shutdown
// ------------
//
void
EpPlatformdMod::mod_shutdown()
{
}

// ep_map_record
// -------------
//
int
EpPlatformdMod::ep_map_record(const ep_map_rec_t *rec)
{
    return EpPlatLibMod::ep_map_record(rec);
    return 0;
}

// ep_unmap_record
// ---------------
//
int
EpPlatformdMod::ep_unmap_record(fds_uint64_t uuid, int idx)
{
    return EpPlatLibMod::ep_unmap_record(uuid, idx);
    return 0;
}

// ep_lookup_rec
// -------------
//
int
EpPlatformdMod::ep_lookup_rec(fds_uint64_t uuid, ep_map_rec_t *out)
{
    return EpPlatLibMod::ep_lookup_rec(uuid, out);
    return 0;
}

int
EpPlatformdMod::ep_lookup_rec(const char *name, ep_map_rec_t *out)
{
    return EpPlatLibMod::ep_lookup_rec(name, out);
    return 0;
}

}  // namespace fds
