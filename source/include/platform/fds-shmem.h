/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_PLATFORM_FDS_SHMEM_H_
#define SOURCE_INCLUDE_PLATFORM_FDS_SHMEM_H_

#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

namespace fds {

class FdsShmem
{
  public:
    FdsShmem(const char *name, size_t size);
    virtual ~FdsShmem();

    void  *shm_alloc();
    void  *shm_attach();
    int   shm_detach();
    int   shm_remove();

  protected:
    const char               *sh_name;
    void                     *sh_addr;
    size_t                    sh_size;
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_PLATFORM_FDS_SHMEM_H_
