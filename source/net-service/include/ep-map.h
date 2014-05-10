/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_NET_SERVICE_INCLUDE_EP_MAP_H_
#define SOURCE_NET_SERVICE_INCLUDE_EP_MAP_H_

#include <netinet/in.h>
#include <fds_module.h>
#include <shared/fds-constants.h>

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

/*
 * POD data in shared memory to record all endpoint/service registrations.
 */
typedef struct ep_map ep_map_t;
struct ep_map
{
    fds_uint32_t             mp_chksum;
    fds_uint32_t             mp_ver;
    fds_uint32_t             mp_rec_cnt;
    ep_map_rec_t             mp_records[MAX_DOMAIN_EP_SVC];
};

class EpPlatLibMod : public Module
{
  public:
    explicit EpPlatLibMod(const char *name);
    virtual ~EpPlatLibMod() {}

    /**
     * Module methods.
     */
    virtual int  mod_init(SysParams const *const p);
    virtual void mod_startup();
    virtual void mod_shutdown();

    virtual int  ep_map_record(const ep_map_rec_t *rec);
    virtual int  ep_unmap_record(fds_uint64_t uuid);
    virtual int  ep_lookup_rec(fds_uint64_t uuid, ep_map_rec_t *out);
    virtual int  ep_lookup_rec(const char *name, ep_map_rec_t *out);

    const ep_map_rec_t *ep_get_rec(int idx);

  protected:
    ep_map_t                *ep_shm_map;
};

class EpPlatformMod : public EpPlatLibMod
{
  public:
    explicit EpPlatformMod(const char *name);
    virtual ~EpPlatformMod() {}

    /**
     * Module methods.
     */
    virtual int  mod_init(SysParams const *const p);
    virtual void mod_startup();
    virtual void mod_shutdown();

    virtual int  ep_map_record(const ep_map_rec_t *rec);
    virtual int  ep_unmap_record(fds_uint64_t uuid);
    virtual int  ep_lookup_rec(fds_uint64_t uuid, ep_map_rec_t *out);
    virtual int  ep_lookup_rec(const char *name, ep_map_rec_t *out);
};

extern EpPlatLibMod          gl_EpPlatLib;
extern EpPlatformMod         gl_EpPlatform;

}  // namespace fds
#endif  // SOURCE_NET_SERVICE_INCLUDE_EP_MAP_H_
