/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include "dm-tvc/VolumeAccessTable.h"

#include "fds_timer.h"

namespace fds
{

std::shared_ptr<FdsTimer>
DmVolumeAccessTable::getTimer() {
    static std::mutex lock;
    static std::weak_ptr<FdsTimer> _timer;

    // Create a timer instance if we need one, we
    // use a weak pointer so the timer can be deleted
    // once all volumes have been closed; i.e. shutdown
    std::lock_guard<std::mutex> g(lock);
    auto ret_val(_timer.lock());
    if (!ret_val) {
        ret_val = std::make_shared<FdsTimer>();
        _timer = ret_val;
    }
    return ret_val;
}

DmVolumeAccessTable::DmVolumeAccessTable(fds_volid_t const vol_uuid)
    : access_map(),
      random_generator(vol_uuid),
      timer(DmVolumeAccessTable::getTimer())
{ }

Error
DmVolumeAccessTable::getToken(fds_int64_t& token,
                              fpi::VolumeAccessMode const& mode,
                              std::chrono::duration<fds_uint32_t> const lease_time) {
    std::unique_lock<std::mutex> lk(lock);
    auto it = access_map.find(token);
    if (access_map.end() == it) {
        // check that there isn't already a writer/cacher to the volume
        if ((mode.can_cache && cached) ||
            (mode.can_write && cached)) {
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
        auto entry = std::make_pair(mode, task);
        timer->schedule(entry.second, lease_time);

        // Update table state
        cached = mode.can_cache;
        access_map[token] = std::move(entry);
    } else {
        // TODO(bszmyd): Thu 09 Apr 2015 10:50:00 AM PDT
        // We currently do not support "upgrading" a token, renewal
        // of a token requires usage of the same mode.
        if (mode != it->second.first) {
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
        timer->cancel(it->second.second);
        if (cached && it->second.first.can_cache) {
            cached = false;
        }
        access_map.erase(it);
    }
}

}  // namespace fds
