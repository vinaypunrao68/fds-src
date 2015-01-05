/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <stdio.h>
#include <platform/fds-shmem.h>
#include <utility>
#include <sys/types.h>
#include <sys/stat.h>
#include <system_error>

#include <util/Log.h>

namespace fds {

FdsShmem::FdsShmem(const char *name, size_t size)
    : sh_name(name), sh_addr(NULL), sh_size(size) {}

FdsShmem::~FdsShmem() {}

void *
FdsShmem::shm_alloc()
{
    int fd = shm_open(sh_name, O_RDWR|O_CREAT| O_EXCL, S_IRWXU);

    // fd should be 0
    if (fd == -1) {
        perror(sh_name);
        return NULL;
    }

    // Truncate the region
    if (ftruncate(fd, sh_size) == -1) {
        perror(sh_name);
        printf("ftruncate....\n");
        return NULL;
    }

    // Map the region
    sh_addr = mmap(NULL, sh_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (sh_addr == NULL) {
        perror(sh_name);
        printf("mmap....\n");
        return NULL;
    }

    // Close the file descriptor, we should no longer need it
    close(fd);

    return sh_addr;
}

void *
FdsShmem::shm_attach(int flags)
{
    int fd;
    // If sh_addr is NULL then we need to try to open the shm segment
    if (sh_addr == NULL) {
        fd = shm_open(sh_name, O_RDWR, S_IRUSR);
        if (fd == -1) {
            std::error_condition econd =
                    std::system_category().default_error_condition(errno);
            GLOGWARN << "Failed to open shared memory segment " << sh_name << "."
                    << " Category: " << econd.category().name() << "."
                    << " errno: " << errno << "."
                    << " Message: " << econd.message() << ".";
            return NULL;
        } else {
            sh_addr = mmap(NULL, sh_size, flags, MAP_SHARED, fd, 0);
        }
        // We should no longer need the file descriptor
        close(fd);
    }

    // Return a pointer to the shared region
    return sh_addr;
}

int
FdsShmem::shm_detach()
{
    // Verify that the shm is mapped first
    if (sh_addr == NULL) {
        return -1;
    }
    return munmap(sh_addr, sh_size);
}

int
FdsShmem::shm_remove()
{
    return shm_unlink(sh_name);
}

}  // namespace fds
