/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <ep-map.h>

namespace fds {

EpPlatLibMod                 gl_EpPlatLib("EP Plat");

EpPlatLibMod::EpPlatLibMod(const char *name)
    : Module(name), ep_shm_map(NULL), ep_mtx("mtx") {}

// mod_init
// --------
//
int
EpPlatLibMod::mod_init(SysParams const *const p)
{
    Module::mod_init(p);

    ep_shm_map = new ep_map();
    memset(ep_shm_map, 0, sizeof(*ep_shm_map));

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
    int idx;

    ep_mtx.lock();
    for (idx = 0; idx < MAX_DOMAIN_EP_SVC; idx++) {
        if (ep_shm_map->mp_records[idx].rmp_uuid == rec->rmp_uuid) {
            /* Found existing slot, overwrite with the new mapping. */
            ep_shm_map->mp_records[idx] = *rec;
            break;
        }
    }
    if (idx == MAX_DOMAIN_EP_SVC) {
        for (idx = 0; idx < MAX_DOMAIN_EP_SVC; idx++) {
            if (ep_shm_map->mp_records[idx].rmp_uuid == 0) {
                /* Use this index */
                ep_shm_map->mp_records[idx] = *rec;
                ep_shm_map->mp_rec_cnt++;
                break;
            }
        }
    }
    fds_verify(ep_shm_map->mp_rec_cnt <= MAX_DOMAIN_EP_SVC);
    ep_mtx.unlock();

    if (idx == MAX_DOMAIN_EP_SVC) {
        idx = -1;
    }
    return idx;
}

// ep_unmap_record
// ---------------
//
int
EpPlatLibMod::ep_unmap_record(fds_uint64_t uuid, int idx)
{
    ep_mtx.lock();
    if (idx == -1) {
        for (idx = 0; idx < MAX_DOMAIN_EP_SVC; idx++) {
            if (ep_shm_map->mp_records[idx].rmp_uuid == uuid) {
                break;
            }
        }
    }
    if (idx < MAX_DOMAIN_EP_SVC) {
        ep_map_rec_t *rec;

        rec = &ep_shm_map->mp_records[idx];
        fds_verify(rec->rmp_uuid == uuid);

        ep_shm_map->mp_rec_cnt--;
        rec->rmp_uuid = 0;
        memset(&rec->rmp_addr, 0, sizeof(rec->rmp_addr));
    }
    fds_verify(ep_shm_map->mp_rec_cnt >= 0);
    ep_mtx.unlock();

    if (idx == MAX_DOMAIN_EP_SVC) {
        idx = -1;
    }
    return idx;
}

// ep_lookup_rec
// -------------
//
int
EpPlatLibMod::ep_lookup_rec(fds_uint64_t uuid, ep_map_rec_t *out)
{
    int idx;

    ep_mtx.lock();
    for (idx = 0; idx < MAX_DOMAIN_EP_SVC; idx++) {
        if (ep_shm_map->mp_records[idx].rmp_uuid == uuid) {
            *out = ep_shm_map->mp_records[idx];
            break;
        }
    }
    ep_mtx.unlock();
    if (idx == MAX_DOMAIN_EP_SVC) {
        memset(out, 0, sizeof(*out));
        idx = -1;
    }
    return idx;
}

int
EpPlatLibMod::ep_lookup_rec(int idx, fds_uint64_t uuid, ep_map_rec_t *out)
{
    if ((idx >= 0) && (idx < MAX_DOMAIN_EP_SVC)) {
        ep_mtx.lock();
        if (ep_shm_map->mp_records[idx].rmp_uuid == uuid) {
            *out = ep_shm_map->mp_records[idx];
        } else {
            idx = -1;
        }
        ep_mtx.unlock();
        if (idx >= 0) {
            return idx;
        }
    }
    memset(out, 0, sizeof(*out));
    return -1;
}

int
EpPlatLibMod::ep_lookup_rec(const char *name, ep_map_rec_t *out)
{
    return 0;
}

}  // namespace fds
