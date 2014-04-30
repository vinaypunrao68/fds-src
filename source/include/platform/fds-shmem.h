/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_PLATFORM_FDS_SHMEM_H_
#define SOURCE_INCLUDE_PLATFORM_FDS_SHMEM_H_


#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/mapped_region.hpp>

namespace fds {

class FdsShmem
{
    typedef boost::interprocess::managed_shared_memory mg_shm;

  public:
    FdsShmem(char *name);
    virtual ~FdsShmem();

    void create_empty(size_t size);
    void  *shm_alloc(size_t size);
    void  *shm_attach();
    int   shm_remove();

  protected:
    char *sh_name;
    mg_shm *sh_segment;
    void *sh_addr;

};

}  // namespace fds
#endif  // SOURCE_INCLUDE_PLATFORM_FDS_SHMEM_H_
