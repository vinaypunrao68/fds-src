/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <platform/fds-shmem.h>

namespace fds {

FdsShmem::FdsShmem(const char *name)
    : sh_name(name), sh_addr(NULL), sh_id(0), sh_size(0) {}

FdsShmem::~FdsShmem() {}

int
FdsShmem::shm_alloc(size_t size)
{
    return 0;
}

void *
FdsShmem::shm_attach()
{
    return NULL;
}

void
FdsShmem::shm_detach()
{
}

}  // namespace fds
