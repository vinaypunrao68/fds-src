/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_NODE_INFO_SHM_ITER_H_
#define SOURCE_INCLUDE_PLATFORM_NODE_INFO_SHM_ITER_H_

namespace fds
{
    /**
     * Iterate through node info records in shared memory.
     */
    class NodeInfoShmIter : public ShmObjIter
    {
        public:
            virtual ~NodeInfoShmIter()
            {
            }

            explicit NodeInfoShmIter(bool rw = false);

            virtual bool
            shm_obj_iter_fn(int idx, const void *k, const void *r, size_t ksz, size_t rsz);

        protected:
            int                         it_no_rec;
            int                         it_no_rec_quit;
            bool                        it_shm_rw;
            const ShmObjRO             *it_shm;
            DomainContainer::pointer    it_local;
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_NODE_INFO_SHM_ITER_H_
