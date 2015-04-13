/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include "dm-tvc/VolumeAccessTable.h"

#include "fds_timer.h"

namespace fds
{

DmVolumeAccessTable::DmVolumeAccessTable(fds_volid_t const vol_uuid)
    : access_map(),
      random_generator(vol_uuid),
      timer(new FdsTimer())
{ }

Error
DmVolumeAccessTable::getToken(fds_int64_t& token,
                              fpi::VolumeAccessPolicy const& policy,
                              std::chrono::duration<fds_uint32_t> const lease_time) {
    std::unique_lock<std::mutex> lk(lock);
    auto it = access_map.find(token);
    if (access_map.end() == it) {
        // check that the exclusivity policy allows for this access request
        if ((policy.exclusive_read && read_locked) ||
            (policy.exclusive_write && write_locked)) {
            return ERR_VOLUME_ACCESS_DENIED;
        }
        // Issue a new access token
        token = random_generator();

        // Start a timer to auto-expire this token
        auto task = boost::shared_ptr<FdsTimerTask>(
            new FdsTimerFunctionTask(*timer,
                                     [this, token] () {
                                        LOGNOTIFY << "Expiring volume token: " << token;
                                        this->removeToken(token);
                                     }));
        auto entry = std::make_pair(policy, task);
        timer->schedule(entry.second, lease_time);

        // Update table state
        read_locked = policy.exclusive_read;
        write_locked = policy.exclusive_write;
        access_map[token] = std::move(entry);
    } else {
        // TODO(bszmyd): Thu 09 Apr 2015 10:50:00 AM PDT
        // We currently do not support "upgrading" a token, renewal
        // of a token requires usage of the same policy.
        if (policy != it->second.first) {
            return ERR_VOLUME_ACCESS_DENIED;
        }
        // Reset the timer for this token
        LOGTRACE << "Renewing volume token: " << token;
        timer->cancel(it->second.second);
        timer->schedule(it->second.second, lease_time);
    }
    return ERR_OK;
}

void
DmVolumeAccessTable::removeToken(fds_int64_t const token) {
    std::unique_lock<std::mutex> lk(lock);
    auto it = access_map.find(token);
    if (access_map.end() != it) {
        if (write_locked && it->second.first.exclusive_write) {
            write_locked = false;
        }
        if (read_locked && it->second.first.exclusive_read) {
            read_locked = false;
        }
        access_map.erase(it);
    }
}

}  // namespace fds
