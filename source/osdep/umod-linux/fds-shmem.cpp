/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <platform/fds-shmem.h>

namespace fds {

FdsShmem::FdsShmem(const char *name)
    : sh_name(name), sh_addr(NULL), sh_size(0) {}


FdsShmem::~FdsShmem() {}

int
FdsShmem::shm_alloc(size_t size)
{
    int fd = shm_open(sh_name, O_RDWR | O_CREAT, S_IRWXU);
    sh_size = size;

    // fd should be 0
    if (fd == -1) {
        return -1;
    }

    // Truncate the region
    if (ftruncate(fd, sh_size) == -1) {
        return -1;
    }

    // Map the region
    sh_addr = mmap(NULL, sh_size, PROT_WRITE, MAP_SHARED, fd, 0);
    if (sh_addr == NULL) {
        return -1;
    }

    // Close the file descriptor, we should no longer need it
    close(fd);

    return 0;
}

void *
FdsShmem::shm_get()
{
    // Verify that the shm_alloc has already been called
    if (sh_addr == NULL) {
        return NULL;
    }

    // Return a pointer to the shared region
    return sh_addr;
}

int
FdsShmem::shm_unmap()
{
    // Verify that the shm is mapped first
    if (sh_addr == NULL) {
        return -1;
    }

    // Unmap the shared region
    return munmap(sh_addr, sh_size);
}
}  // namespace fds
