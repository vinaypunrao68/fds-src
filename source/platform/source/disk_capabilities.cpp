/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include "platform/disk_capabilities.h"

extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
#include <glob.h>
#include <fcntl.h>
#include <unistd.h>
}

#include "util/Log.h"
#include "fds_assert.h"
#include "disk_capability.h"

namespace fds
{

void
DiskCapabilitiesMgr::dsk_capability_read(PmDiskObj::pointer disk) {
    DiskCapability   *capability = disk->dsk_xfer_capability();

    if (capability == NULL)
    {
        disk->dsk_read_capability();
        capability = disk->dsk_xfer_capability();
        fds_assert(capability != NULL);
    }

    fds_verify(capability->dc_owner == disk);

    dc_mtx.lock();
    dc_capabilities.chain_add_back(&capability->dc_link);
    dc_mtx.unlock();
}

std::pair<size_t, size_t>
DiskCapabilitiesMgr::disk_capacities() {
    ChainIter iter;
    auto ret_val = std::make_pair(0, 0);

    dc_mtx.lock();
    chain_foreach(&dc_capabilities, iter) {
        auto capability = dc_capabilities.chain_iter_current<DiskCapability>(iter);

        if (!capability->initialized) {
            continue;
        }

        if (capability->spinning) {
            ret_val.first += capability->capacity_gb;
        } else {
            ret_val.second += capability->capacity_gb;
        }
    }
    dc_mtx.unlock();
    return ret_val;
}


std::pair<size_t, size_t>
DiskCapabilitiesMgr::disk_counts() {
    ChainIter iter;
    auto ret_val = std::make_pair(0, 0);

    dc_mtx.lock();
    chain_foreach(&dc_capabilities, iter) {
        auto capability = dc_capabilities.chain_iter_current<DiskCapability>(iter);

        if (!capability->initialized) {
            continue;
        }

        if (capability->spinning) {
            ++ret_val.first;
        } else {
            ++ret_val.second;
        }
    }
    dc_mtx.unlock();
    return ret_val;
}


fds_disk_type_t
DiskCapabilitiesMgr::disk_type() {
    fds_disk_type_t ret_val = FDS_DISK_SATA;
    ChainIter iter;

    dc_mtx.lock();
    chain_foreach(&dc_capabilities, iter) {
        auto capability = dc_capabilities.chain_iter_current<DiskCapability>(iter);

        if (!capability->initialized) {
            continue;
        }
        ret_val = capability->disk_type;
        break;
    }
    dc_mtx.unlock();
    return ret_val;
}

}  // namespace fds
