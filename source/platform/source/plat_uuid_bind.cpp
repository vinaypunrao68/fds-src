/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include "plat_uuid_bind.h"
#include "plat_uuid_bind_update.h"

namespace fds
{
    // shmq_handler
    // ------------
    //
    void PlatUuidBind::shmq_handler(const shmq_req_t *in, size_t size)
    {
        int                    idx;
        ep_shmq_req_t         *out;
        fds_threadpool        *thr;
        ShmConPrdQueue        *plat;
        const ep_shmq_req_t   *ep_map = reinterpret_cast<const ep_shmq_req_t *>(in);

        /* Save the uuid to physical binding info to the node shared memory. */
        idx = ep_shm_rw->ep_map_record(&ep_map->smq_rec);
        fds_verify(idx != -1);

        /* Cache the binding info. */
        NetMgr::ep_mgr_singleton()->ep_register_binding(&ep_map->smq_rec, idx);

        /* Relay same info from 'in' to 'out' with the new index. */
        out           = new ep_shmq_req_t;
        out->smq_hdr  = *in;
        out->smq_idx  = idx;
        out->smq_type = ep_map->smq_type;
        out->smq_rec  = ep_map->smq_rec;

        /* Reply the mapping result back to the sender and all other services. */
        plat = NodeShmCtrl::shm_producer();
        plat->shm_producer(static_cast<void *>(out), sizeof(*out), 0);

        /* Iterate through each platform node in the domain & update the binding. */
        PlatUuidBindUpdate::pointer    iter = new PlatUuidBindUpdate();

        thr = NetMgr::ep_mgr_thrpool();
        thr->schedule(&PlatUuidBindUpdate::uuid_bind_update, iter, out);
    }
}  // namespace fds
