/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_PLAT_UUID_BIND_H_
#define SOURCE_PLATFORM_INCLUDE_PLAT_UUID_BIND_H_

#include <ep-map.h>

namespace fds
{
    /*
     * -------------------------------------------------------------------------------------
     * Platform deamon shared memory queue handlers
     * -------------------------------------------------------------------------------------
     */
    class PlatUuidBind : public ShmqReqIn
    {
        public:
            explicit PlatUuidBind(EpPlatformdMod *rw) : ShmqReqIn(), ep_shm_rw(rw)
            {
            }

            virtual ~PlatUuidBind()
            {
            }

            void shmq_handler(const shmq_req_t *in, size_t size);

        private:
            EpPlatformdMod   *ep_shm_rw;
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_PLAT_UUID_BIND_H_
