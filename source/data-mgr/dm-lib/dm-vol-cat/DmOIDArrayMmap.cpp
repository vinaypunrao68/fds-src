/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <dm-vol-cat/DmOIDArrayMmap.h>

namespace fds {
const fds_uint32_t DmOIDArrayMmap::FRAGMENT_SIZE = 102400;
const fds_uint32_t DmOIDArrayMmap::NUM_OBJS_PER_FRAGMENT =
        DmOIDArrayMmap::FRAGMENT_SIZE / OBJECTID_DIGESTLEN;

DmOIDArrayMmap::DmOIDArrayMmap(fds_uint64_t id, int fd) : id_(id), fd_(fd) {
    fds_verify(fd_ >= 0);
    base_ = 0;

    // XXX: Construction of an object is not thread-safe. Once the object
    // is created, all functions are thread-safe.

    struct stat64 st = {0};
    int rc = fstat64(fd_, &st);
    fds_verify(0 == rc);

    off64_t endOffset = (id_ + 1) * FRAGMENT_SIZE - 1;
    if (st.st_size < endOffset) {
        char zero = 0;
        rc = pwrite64(fd_, reinterpret_cast<void *>(&zero), 1, endOffset);
        fds_verify(1 == rc);
    }

    base_ = mmap64(NULL, FRAGMENT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
            fd_, id_ * FRAGMENT_SIZE);
    fds_verify(valid());
}

DmOIDArrayMmap::~DmOIDArrayMmap() {
    if (valid()) {
        munmap(base_, FRAGMENT_SIZE);
    }
}

}  // namespace fds
