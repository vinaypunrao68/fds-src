/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <ep-map.h>
#include <platform/node-inv-shmem.h>

namespace fds {

static EpPlatLibMod          gl_EpShmSharedLib("Ep PlatShm Lib");
EpPlatLibMod                *gl_EpShmPlatLib = &gl_EpShmSharedLib;

EpPlatLibMod::EpPlatLibMod(const char *name) : Module(name), ep_uuid_bind(NULL)
{
    static Module *ep_plat_dep_mods[] = {
        &gl_NodeShmCtrl,
        NULL
    };
    mod_intern = ep_plat_dep_mods;
}

// mod_init
// --------
//
int
EpPlatLibMod::mod_init(SysParams const *const p)
{
    return Module::mod_init(p);
}

// mod_startup
// -----------
//
void
EpPlatLibMod::mod_startup()
{
    Module::mod_startup();
    ep_uuid_bind = NodeShmCtrl::shm_uuid_binding();
}

// mod_shutdown
// ------------
//
void
EpPlatLibMod::mod_shutdown()
{
    Module::mod_shutdown();
}

// ep_map_record
// -------------
//
int
EpPlatLibMod::ep_map_record(const ep_map_rec_t *rec)
{
    std::cout << "map_record: need to hookup with shm queue" << std::endl;
    return 0;
}

// ep_unmap_record
// ---------------
//
int
EpPlatLibMod::ep_unmap_record(fds_uint64_t uuid, int idx)
{
    std::cout << "Need to hookup with shm queue" << std::endl;
    return 0;
}

// ep_lookup_rec
// -------------
//
int
EpPlatLibMod::ep_lookup_rec(fds_uint64_t uuid, ep_map_rec_t *out)
{
    return ep_uuid_bind->shm_lookup_rec(static_cast<void *>(&uuid),
                                        static_cast<void *>(out), sizeof(*out));
}

int
EpPlatLibMod::ep_lookup_rec(int idx, fds_uint64_t uuid, ep_map_rec_t *out)
{
    return ep_uuid_bind->shm_lookup_rec(idx, static_cast<void *>(&uuid),
                                        static_cast<void *>(out), sizeof(*out));
}

int
EpPlatLibMod::ep_lookup_rec(const char *name, ep_map_rec_t *out)
{
    return 0;
}

}  // namespace fds
