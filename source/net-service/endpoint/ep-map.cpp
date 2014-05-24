/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <ep-map.h>
#include <platform/platform-lib.h>

namespace fds {

static EpPlatLibMod          gl_EpShmSharedLib("Ep PlatShm Lib");
EpPlatLibMod                *gl_EpShmPlatLib = &gl_EpShmSharedLib;

EpPlatLibMod::EpPlatLibMod(const char *name) : Module(name), ep_uuid_bind(NULL)
{
    static Module *ep_plat_dep_mods[] = {
        &gl_NodeShmROCtrl,
        NULL
    };
    ep_my_type = 0;
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
// Must make sure the service type enum < max. number of consumers in the queue.
//
void
EpPlatLibMod::mod_startup()
{
    Module::mod_startup();
    ep_uuid_bind = NodeShmCtrl::shm_uuid_binding();
    ep_my_type   = Platform::platf_singleton()->plf_get_node_type();
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
    return ep_req_map_record(SMQ_UUID_BIND_REQ, rec);
}

// ep_unmap_record
// ---------------
//
int
EpPlatLibMod::ep_unmap_record(fds_uint64_t uuid, int idx)
{
    return 0;
}

// ep_req_map_record
// -----------------
//
int
EpPlatLibMod::ep_req_map_record(fds_uint32_t op, const ep_map_rec_t *rec)
{
    ep_shmq_req_t   req;
    ShmConPrdQueue *queue;

    req.smq_code = op;
    req.smq_idx  = -1;
    req.smq_rec  = *rec;

    queue = NodeShmCtrl::shm_producer();
    queue->shm_producer(static_cast<void *>(&req), sizeof(req), ep_my_type);

    // XXX: block here for the mapping to come back.

    return 0;

    // Panic for now...
    fds_verify(req.smq_idx != -1);
    return req.smq_idx;
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
