/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <platform/fds-shmem.h>
#include <stdio.h>
#include <utility>
#include <sys/types.h>
#include <sys/stat.h>
namespace fds {

FdsShmem::FdsShmem(char *name)
        : sh_segment(nullptr),
          sh_name(name),
          sh_addr(nullptr) {}


FdsShmem::~FdsShmem() {}

boost::interprocess::managed_shared_memory *
FdsShmem::shm_create_empty(size_t size)
{
    // Close existing shm if there is one
    boost::interprocess::shared_memory_object::remove(sh_name);

    // Create the segment
    sh_segment = new boost::interprocess::managed_shared_memory
            (boost::interprocess::create_only, sh_name, size+1024);

    return sh_segment;
}

boost::interprocess::managed_shared_memory::segment_manager *
FdsShmem::shm_get_mgr()
{
    return sh_segment->get_segment_manager();
}

void *
FdsShmem::shm_alloc(size_t size)
{
    // Close existing shm if there is one
    boost::interprocess::shared_memory_object::remove(sh_name);

    // Create the segment
    sh_segment = new boost::interprocess::managed_shared_memory
            (boost::interprocess::create_only, sh_name, size+1024);

    // Create a new byte array for usable space
    sh_addr = sh_segment->construct<unsigned char>("shm_obj")[size](0);

    return sh_addr;
}

void *
FdsShmem::shm_attach()
{
    if (sh_segment == nullptr) {
        // Connect to shmem
        sh_segment = new boost::interprocess::managed_shared_memory
                (boost::interprocess::open_only, sh_name);
    }
    std::pair<unsigned char*, std::size_t> res =
            sh_segment->find<unsigned char>("shm_obj");

    if (res.first == nullptr) {
        return nullptr;
    }

    sh_addr = res.first;

    return sh_addr;
}

bool
FdsShmem::shm_remove()
{
    if (sh_addr != nullptr) {
        sh_segment->destroy_ptr(sh_addr);
        sh_addr = nullptr;
    }
    return boost::interprocess::
            shared_memory_object::remove(sh_name);
}

}  // namespace fds
