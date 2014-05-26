/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <ep-map.h>
#include <platform.h>

namespace fds {

/*
 * -------------------------------------------------------------------------------------
 * Platform deamon shared memory queue handlers
 * -------------------------------------------------------------------------------------
 */
class PlatUuidBind : public ShmqReqIn
{
  public:
    explicit PlatUuidBind(EpPlatformdMod *rw) : ShmqReqIn(), ep_shm_rw(rw) {}
    virtual ~PlatUuidBind() {}

    void shmq_handler(const shmq_req_t *in, size_t size);

  private:
    EpPlatformdMod          *ep_shm_rw;
};

// shmq_handler
// ------------
//
void
PlatUuidBind::shmq_handler(const shmq_req_t *in, size_t size)
{
    int                  idx;
    ep_shmq_req_t        out;
    ShmConPrdQueue      *plat;
    const ep_shmq_req_t *ep_map = reinterpret_cast<const ep_shmq_req_t *>(in);

    std::cout << "UUID binding request is called" << std::endl;
    idx = ep_shm_rw->ep_map_record(&ep_map->smq_rec);
    fds_verify(idx != -1);

    /* Relay the header from 'in' to 'out'. */
    out.smq_hdr = *in;

    plat = NodeShmCtrl::shm_producer();
    plat->shm_producer(static_cast<void *>(&out), sizeof(out), 0);
}

static PlatUuidBind *platform_uuid_bind;

/*
 * -------------------------------------------------------------------------------------
 * Platform NetEndPoint services
 * -------------------------------------------------------------------------------------
 */
EpPlatformdMod::EpPlatformdMod(const char *name) : EpPlatLibMod(name)
{
    static Module *ep_plat_dep_mods[] = {
        &gl_NodeShmRWCtrl,
        NULL
    };
    mod_intern = ep_plat_dep_mods;
    platform_uuid_bind = new PlatUuidBind(this);
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

// mod_enable_service
// ------------------
//
void
EpPlatformdMod::mod_enable_service()
{
    ShmConPrdQueue *consumer;

    consumer = NodeShmCtrl::shm_consumer();
    consumer->shm_register_handler(SHMQ_REQ_UUID_BIND, platform_uuid_bind);
    consumer->shm_register_handler(SHMQ_REQ_UUID_UNBIND, platform_uuid_bind);
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
