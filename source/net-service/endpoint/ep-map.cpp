/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <ep-map.h>
#include <platform/platform-lib.h>

namespace fds {

static EpPlatLibMod          gl_EpShmSharedLib("Ep PlatShm Lib");
EpPlatLibMod                *gl_EpShmPlatLib = &gl_EpShmSharedLib;

/*
 * -------------------------------------------------------------------------------------
 * Platform lib shared memory queue handlers
 * -------------------------------------------------------------------------------------
 */
class PlatLibUuidBind : public ShmqReqIn
{
  public:
    PlatLibUuidBind() : ShmqReqIn() {}
    virtual ~PlatLibUuidBind() {}

    void shmq_handler(const shmq_req_t *in, size_t size) override;
};

static PlatLibUuidBind  platlib_uuid_bind;

/*
 * -------------------------------------------------------------------------------------
 * Platform Library NetEndPoint services
 * -------------------------------------------------------------------------------------
 */
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
    ep_node_bind = NodeShmCtrl::shm_node_inventory();
    ep_am_bind   = NodeShmCtrl::shm_am_inventory();
    ep_my_type   = Platform::platf_singleton()->plf_get_node_type();
}

// mod_enable_service
// ------------------
//
void
EpPlatLibMod::mod_enable_service()
{
    ShmConPrdQueue *consumer;

    consumer = NodeShmCtrl::shm_consumer();
    consumer->shm_register_handler(SHMQ_REQ_UUID_BIND, &platlib_uuid_bind);
    consumer->shm_register_handler(SHMQ_REQ_UUID_UNBIND, &platlib_uuid_bind);
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
    return ep_req_map_record(SHMQ_REQ_UUID_BIND, rec);
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
    ShmConPrdQueue *mine, *plat;
    ep_shmq_req_t   reqt, resp;
    ShmqReqOut      out(true, &resp.smq_hdr, sizeof(resp));

    resp.smq_idx  = -1;
    reqt.smq_idx  = -1;
    reqt.smq_rec  = *rec;
    reqt.smq_type = SHM_TAB_NONE;
    reqt.smq_hdr.smq_code = op;
    resp.smq_rec.rmp_uuid = 0;

    plat = NodeShmCtrl::shm_producer();
    mine = NodeShmCtrl::shm_consumer();

    mine->shm_track_request(&out, &reqt.smq_hdr);
    plat->shm_producer(static_cast<void *>(&reqt), sizeof(reqt), ep_my_type);

    // Block for the mapping to come back.
    out.req_wait();

    // Panic for now...
    fds_verify(resp.smq_idx != -1);
    fds_verify(resp.smq_rec.rmp_uuid == rec->rmp_uuid);
    return resp.smq_idx;
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

int
EpPlatLibMod::node_info_lookup(int idx, fds_uint64_t node_uuid, ep_map_rec_t *out)
{
    int          ret;
    node_data_t  ninfo;

    fds_verify(idx >= 0);
    ret = ep_node_bind->shm_lookup_rec(idx, static_cast<void *>(&node_uuid),
                                       static_cast<void *>(&ninfo), sizeof(ninfo));
    if (idx == -1) {
        idx = ep_am_bind->shm_lookup_rec(idx, static_cast<void *>(&node_uuid),
                                         static_cast<void *>(&ninfo), sizeof(ninfo));
    } else {
        idx = ret;
    }
    if (idx == -1) {
        return idx;
    }
    ep_node_info_to_mapping(&ninfo, out);
    return idx;
}

// ep_node_info_to_mapping
// -----------------------
//
/* static */ void
EpPlatLibMod::ep_node_info_to_mapping(const node_data_t *src, ep_map_rec_t *dest)
{
    struct sockaddr    *adr;
    struct sockaddr_in *ip4;

    dest->rmp_uuid = src->nd_node_uuid;
    strncpy(dest->rmp_name, src->nd_assign_name, sizeof(dest->rmp_name));

    /* Do IPv4 for now */
    adr = &dest->rmp_addr;
    ip4 = reinterpret_cast<struct sockaddr_in *>(adr);

    adr->sa_family = AF_INET;
    ip4->sin_port  = src->nd_base_port;
    inet_ntop(AF_INET, src->nd_ip_addr,
              reinterpret_cast<char *>(&ip4->sin_addr), INET_ADDRSTRLEN);
}

// ep_uuid_bind_to_msg
// -------------------
//
/* static */ void
EpPlatLibMod::ep_uuid_bind_to_msg(const ep_map_rec_t *src, fpi::UuidBindMsg *msg)
{
}

/*
 * -------------------------------------------------------------------------------------
 * Platform lib shared memory queue handlers
 * -------------------------------------------------------------------------------------
 */
void
PlatLibUuidBind::shmq_handler(const shmq_req_t *in, size_t size)
{
    std::cout << "Plat lib uuid binding is called " << std::endl;
}

}  // namespace fds
