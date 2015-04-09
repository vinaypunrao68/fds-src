/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include "dm-tvc/VolumeAccessTable.h"

namespace fds
{

Error DmVolumeAccessTable::getToken(fds_int64_t& token,
                                   fpi::VolumeAccessPolicy const& policy)
{
    auto it = access_map.find(token);
    if (access_map.end() == it) {
        // check that the exclusivity policy allows for this access request
        if ((policy.exclusive_read && read_locked) ||
            (policy.exclusive_write && write_locked)) {
            return ERR_VOLUME_ACCESS_DENIED;
        }
        token = random_generator();
        access_map[token] = policy;
    } else {
        // token is already registered, just reset the timer
    }
    return ERR_OK;
}

void DmVolumeAccessTable::removeToken(fds_int64_t const token) {
    auto it = access_map.find(token);
    if (access_map.end() != it) {
        if (write_locked && it->second.exclusive_write) {
            write_locked = false;
        }
        if (read_locked && it->second.exclusive_read) {
            read_locked = false;
        }
        access_map.erase(it);
    }
}

}  // namespace fds
