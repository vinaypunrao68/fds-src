/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <ep-map.h>
#include <platform/platform-lib.h>

namespace fds {

static EpPlatLibMod          gl_EpShmSharedLib("Ep PlatShm Lib");
EpPlatLibMod                *gl_EpShmPlatLib = &gl_EpShmSharedLib;

/* Make sure keys are contiguous */
cc_assert(m1, offsetof(ep_map_rec_t, rmp_uuid) == 0);
cc_assert(m3, offsetof(ep_map_rec_t, rmp_addr) == EP_MAP_REC_KEY_SZ);

/*
 * -------------------------------------------------------------------------------------
 * Platform lib shared memory queue handlers
 * -------------------------------------------------------------------------------------
 */
class PlatLibUuidBind : public ShmqReqIn
{
  public:
    void shmq_handler(const shmq_req_t *in, size_t size) override;
};

class PlatLibNodeReg : public ShmqReqIn
{
  public:
    void shmq_handler(const shmq_req_t *in, size_t size) override;
};

class PlatLibDltUpdate : public ShmqReqIn
{
  public:
    void shmq_handler(const shmq_req_t *in, size_t size) override {
       LOGNORMAL << " get a dlt update msg from shmq ";
    }
};

static PlatLibNodeReg        platlib_node_reg;
static PlatLibUuidBind       platlib_uuid_bind;
static PlatLibDltUpdate      platlib_dlt_update;

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
    consumer->shm_register_handler(SHMQ_NODE_REGISTRATION, &platlib_node_reg);
    consumer->shm_register_handler(SHMQ_DLT_UPDATE, &platlib_dlt_update);
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
// This is the main entry to the uuid binding update protocol.
//
// Binding Service  :  Local Platformd      :    Peer Platformds   :   Peer Services
// ---------------  :  ---------------      :    ---------------   :   -------------
// ep_req_map_record                        :                      :
//     -- shmq req --> PlatUuidBind::shmq_handler (net-svc/ep-map.cpp)
//                  :    o record binding   :                      :
//     <-- shmq resp --  o send back response                      :
// Done             :    o update peer Platformds                  :
//                  :  PlatUuidBindUpdate::uuid_bind_update        :
//                  :             -- message --> PlatformEpHandler::allUuidBinding
//                  :                       :    (net-svc/net-registration.cpp)
//                  :                       :      o record binding.
//                  :                       :      o update local services
//                  :                       :            --- shmq req -->
//                  :                       :                      : PlatLibUuidBind::
//                  :                       :                      : shmq_handler
int
EpPlatLibMod::ep_req_map_record(fds_uint32_t op, const ep_map_rec_t *rec)
{
    ShmConPrdQueue *mine, *plat;
    ep_shmq_req_t   reqt, resp;
    ResourceUUID    uuid;
    ShmqReqOut      out(true, &resp.smq_hdr, sizeof(resp));

    return -1;
    uuid.uuid_set_from_raw(reinterpret_cast<const fds_uint8_t *>(&rec->rmp_uuid), false);
    reqt.smq_type = uuid.uuid_get_type();
    if (RsType::rs_func(reqt.smq_type, RsType::RS_IN_NODE_INV) == true) {
        int          idx;
        ep_map_rec_t svc;

        idx = node_info_lookup(rec->rmp_uuid, &svc);
        fds_assert(idx != -1);
        fds_assert(rec->rmp_uuid == svc.rmp_uuid);
        return idx;
    }
    resp.smq_idx  = -1;
    reqt.smq_idx  = -1;
    reqt.smq_rec  = *rec;
    reqt.smq_hdr.smq_code = op;
    resp.smq_rec.rmp_uuid = 0;

    plat = NodeShmCtrl::shm_producer();
    mine = NodeShmCtrl::shm_consumer();
    mine->shm_track_request(&out, &reqt.smq_hdr, ep_my_type);
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
EpPlatLibMod::ep_lookup_rec(fds_uint64_t  uuid,
                            fds_uint32_t  maj,
                            fds_uint32_t  min,
                            ep_map_rec_t *out)
{
    ep_map_rec_t key;

    key.rmp_uuid  = uuid;
    key.rmp_major = maj;
    key.rmp_minor = min;
    return ep_uuid_bind->shm_lookup_rec(static_cast<void *>(&key),
                                        static_cast<void *>(out), sizeof(*out));
}

int
EpPlatLibMod::ep_lookup_rec(int           idx,
                            fds_uint64_t  uuid,
                            fds_uint32_t  maj,
                            fds_uint32_t  min,
                            ep_map_rec_t *out)
{
    ep_map_rec_t key;

    key.rmp_uuid  = uuid;
    key.rmp_major = maj;
    key.rmp_minor = min;
    return ep_uuid_bind->shm_lookup_rec(idx, static_cast<void *>(&key),
                                        static_cast<void *>(out), sizeof(*out));
}

int
EpPlatLibMod::ep_lookup_rec(const char   *name,
                            fds_uint32_t  maj,
                            fds_uint32_t  min,
                            ep_map_rec_t *out)
{
    ep_map_rec_t key;

    key.rmp_major = maj;
    key.rmp_minor = min;
    return 0;
}

// mod_info_lookup
// ---------------
//
int
EpPlatLibMod::node_info_lookup(int idx, fds_uint64_t node_uuid, ep_map_rec_t *out)
{
    int          ret;
    node_data_t  ninfo;

    fds_verify(idx >= 0);
    ret = ep_node_bind->shm_lookup_rec(idx, static_cast<const void *>(&node_uuid),
                                       static_cast<void *>(&ninfo), sizeof(ninfo));
    if (ret == -1) {
        ret = ep_am_bind->shm_lookup_rec(idx, static_cast<const void *>(&node_uuid),
                                         static_cast<void *>(&ninfo), sizeof(ninfo));
        if (ret == -1) {
            return -1;
        }
    }
    fds_assert(idx == ret);
    ep_node_info_to_mapping(&ninfo, out);

    return idx;
}

int
EpPlatLibMod::node_info_lookup(fds_uint64_t node_uuid, ep_map_rec_t *out)
{
    int          idx;
    node_data_t  ninfo;

    idx = ep_node_bind->shm_lookup_rec(static_cast<const void *>(&node_uuid),
                                       static_cast<void *>(&ninfo), sizeof(ninfo));
    if (idx == -1) {
        idx = ep_am_bind->shm_lookup_rec(static_cast<const void *>(&node_uuid),
                                         static_cast<void *>(&ninfo), sizeof(ninfo));
        if (idx == -1) {
            return idx;
        }
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
    dest->rmp_uuid  = src->nd_node_uuid;
    dest->rmp_major = 0;
    dest->rmp_minor = 0;
    strncpy(dest->rmp_name, src->nd_assign_name, sizeof(dest->rmp_name));
    EpAttr::netaddr_frm_str(&dest->rmp_addr, src->nd_base_port, src->nd_ip_addr);
}

// ep_uuid_bind_to_msg
// -------------------
//
/* static */ void
EpPlatLibMod::ep_uuid_bind_to_msg(const ep_map_rec_t *src, fpi::UuidBindMsg *msg)
{
    Platform *plat;
    char      buf[INET6_ADDRSTRLEN + 1];

    EpAttr::netaddr_to_str(&src->rmp_addr, buf, INET6_ADDRSTRLEN);
    msg->svc_addr.assign(buf);
    msg->svc_id.svc_name.assign(src->rmp_name);

    msg->svc_port = EpAttr::netaddr_get_port(&src->rmp_addr);
    msg->svc_id.svc_uuid.svc_uuid = src->rmp_uuid;

    plat = Platform::platf_singleton();
    msg->svc_type                   = plat->plf_get_node_type();
    msg->svc_auto_name              = *plat->plf_get_auto_node_name();
    msg->svc_node.svc_name          = *plat->plf_get_my_name();
    msg->svc_node.svc_uuid.svc_uuid = plat->plf_get_my_node_uuid()->uuid_get_val();
}

// ep_uuid_bind_frm_msg
// --------------------
//
/* static */ void
EpPlatLibMod::ep_uuid_bind_frm_msg(ep_map_rec_t *rec, const fpi::UuidBindMsg *msg)
{
    rec->rmp_uuid  = msg->svc_id.svc_uuid.svc_uuid;
    rec->rmp_major = 0;
    rec->rmp_minor = 0;
    EpAttr::netaddr_frm_str(&rec->rmp_addr, msg->svc_port, msg->svc_addr.c_str());
    strncpy(rec->rmp_name, msg->svc_id.svc_name.c_str(), sizeof(rec->rmp_name));
}

/*
 * -------------------------------------------------------------------------------------
 * Platform lib shared memory queue handlers
 * -------------------------------------------------------------------------------------
 */

// This function is notified from EpPlatformMod::node_reg_notify() through shared
// memory queue interface.
//
void
PlatLibUuidBind::shmq_handler(const shmq_req_t *in, size_t size)
{
    const ep_shmq_req_t      *map = reinterpret_cast<const ep_shmq_req_t *>(in);
    NodeAgent::pointer        agent;
    DomainContainer::pointer  local;

    std::cout << "Plat lib uuid binding is called " << std::endl;
    return;

    /* Cache the binding info. */
    fds_assert(map->smq_idx >= 0);
    NetMgr::ep_mgr_singleton()->ep_register_binding(&map->smq_rec, map->smq_idx);
}

void
PlatLibNodeReg::shmq_handler(const shmq_req_t *in, size_t size)
{
    NodeAgent::pointer        agent;
    DomainContainer::pointer  local;
    ShmObjROKeyUint64        *shm;
    const ep_shmq_node_req_t *req = reinterpret_cast<const ep_shmq_node_req_t *>(in);

    /* Do consistency check */
    shm = NodeShmCtrl::shm_node_inventory(req->smq_type);

    /* Register the node to platform lib domain. */
    agent = NULL;
    local = Platform::platf_singleton()->plf_node_inventory();
    local->dc_register_node(shm, &agent, req->smq_idx, -1, NODE_DO_PROXY_ALL_SVCS);
}

}  // namespace fds
