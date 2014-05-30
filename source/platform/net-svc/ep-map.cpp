/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <ep-map.h>
#include <platform.h>

namespace fds {

EpPlatformdMod::EpPlatformdMod(const char *name) : EpPlatLibMod(name)
{
    static Module *ep_plat_dep_mods[] = {
        &gl_NodeShmRWCtrl,
        NULL
    };
    mod_intern = ep_plat_dep_mods;
}

// mod_startup
// -----------
//
void
EpPlatformdMod::mod_startup()
{
    Module::mod_startup();
    ep_uuid_rw   = NodeShmRWCtrl::shm_uuid_rw_binding();
    ep_uuid_bind = NodeShmRWCtrl::shm_uuid_binding();
}

// ep_map_record
// -------------
//
int
EpPlatformdMod::ep_map_record(const ep_map_rec_t *rec)
{
    std::cout << "Platform ep map record " << rec->rmp_uuid << std::endl;
    return ep_uuid_rw->shm_insert_rec(static_cast<const void *>(&rec->rmp_uuid),
                                      static_cast<const void *>(rec), sizeof(*rec));
}

// ep_unmap_record
// ---------------
//
int
EpPlatformdMod::ep_unmap_record(fds_uint64_t uuid, int idx)
{
    return ep_uuid_rw->shm_remove_rec(idx, static_cast<const void *>(&uuid), NULL, 0);
}

}  // namespace fds
