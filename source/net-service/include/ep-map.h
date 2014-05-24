/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_NET_SERVICE_INCLUDE_EP_MAP_H_
#define SOURCE_NET_SERVICE_INCLUDE_EP_MAP_H_

#include <netinet/in.h>
#include <string>
#include <fds-shmobj.h>
#include <net/net-service.h>
#include <shared/fds-constants.h>
#include <platform/node-inv-shmem.h>

namespace fds {

/*
 * POD data in shared memory to map uuid to the right physical entity.
 */
typedef struct ep_map_rec ep_map_rec_t;
struct ep_map_rec
{
    struct sockaddr          rmp_addr;
    fds_uint64_t             rmp_uuid;
    char                     rmp_name[MAX_SVC_NAME_LEN];
};

/**
 * Item to put in shared memory queue.
 */
typedef struct ep_shmq_req
{
    NODE_SHM_QUEUE_ITEM_FIELDS;
    int                      smq_idx;
    ep_map_rec_t             smq_rec;
} ep_shmq_req_t;

cc_assert(ep_map0, sizeof(ep_shmq_req_t) <= sizeof(node_shm_queue_item_t));

class EpPlatLibMod : public Module
{
  public:
    explicit EpPlatLibMod(const char *name);
    virtual ~EpPlatLibMod() {}

    // Module methods.
    //
    virtual int  mod_init(SysParams const *const p) override;
    virtual void mod_startup() override;
    virtual void mod_shutdown() override;

    virtual int  ep_map_record(const ep_map_rec_t *rec);
    virtual int  ep_unmap_record(fds_uint64_t uuid, int idx);
    virtual int  ep_lookup_rec(fds_uint64_t uuid, ep_map_rec_t *out);
    virtual int  ep_lookup_rec(int idx, fds_uint64_t uuid, ep_map_rec_t *out);
    virtual int  ep_lookup_rec(const char *name, ep_map_rec_t *out);

    inline const ep_map_rec_t *ep_get_rec(int idx) {
        return ep_uuid_bind->shm_get_rec<ep_map_rec_t>(idx);
    }

  protected:
    int                      ep_my_type;
    ShmObjROKeyUint64       *ep_uuid_bind;

    int ep_req_map_record(fds_uint32_t op, const ep_map_rec_t *rec);
};

class EpPlatformdMod : public EpPlatLibMod
{
  public:
    explicit EpPlatformdMod(const char *name);
    virtual ~EpPlatformdMod() {}

    virtual void mod_startup() override;
    virtual int  ep_map_record(const ep_map_rec_t *rec) override;
    virtual int  ep_unmap_record(fds_uint64_t uuid, int idx) override;

  protected:
    ShmObjRWKeyUint64       *ep_uuid_rw;
};

extern EpPlatLibMod         *gl_EpShmPlatLib;

}  // namespace fds
#endif  // SOURCE_NET_SERVICE_INCLUDE_EP_MAP_H_
