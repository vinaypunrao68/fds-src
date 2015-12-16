/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include "dm-tvc/VolumeAccessTable.h"

#include "util/Log.h"
#include "fds_timer.h"

namespace fds
{

boost::shared_ptr<FdsTimer>
DmVolumeAccessTable::getTimer() {
    auto timer = MODULEPROVIDER()->getTimer();
    return timer;
}

DmVolumeAccessTable::DmVolumeAccessTable(fds_volid_t const vol_uuid)
    : access_map(),
      random_generator(vol_uuid.get()),
      timer(DmVolumeAccessTable::getTimer())
{ }

Error
DmVolumeAccessTable::getToken(fpi::SvcUuid const& client_uuid,
                              fds_int64_t& token,
                              fpi::VolumeAccessMode const& mode,
                              std::chrono::duration<fds_uint32_t> const lease_time) {
    std::unique_lock<std::mutex> lk(lock);
    auto it = access_map.find(token);
    if (access_map.end() == it) {
        // check that there isn't already a writer/cacher to the volume
        if ((mode.can_cache && cached) ||
            (mode.can_write && cached)) {
            // Ok...find a token for the client that's asking for it, maybe
            // she forget what it was?
            it = std::find_if(access_map.begin(),
                              access_map.end(),
                              [client_uuid] (std::pair<fds_int64_t, DmVolumeAccessEntry> const& e) -> bool {
                                  return e.second.client_uuid == client_uuid;
                              });
            if (access_map.end() == it) {
                return ERR_VOLUME_ACCESS_DENIED;
            } else {
                token = it->first;
            }
        }
    }

    if (access_map.end() == it) {
        // Issue a new access token
        token = random_generator();

        // Start a timer to auto-expire this token
        auto task = boost::shared_ptr<FdsTimerTask>(
            new FdsTimerFunctionTask(*timer,
                                     [this, token] () {
                                        LOGNOTIFY << "Expiring volume token: " << token;
                                        this->removeToken(token);
                                     }));
        timer->schedule(task, lease_time);

        // Update table state
        cached = mode.can_cache;
        access_map[token] = DmVolumeAccessEntry { mode, client_uuid, task };
    } else {
        // TODO(bszmyd): Thu 09 Apr 2015 10:50:00 AM PDT
        // We currently do not support "upgrading" a token, renewal
        // of a token requires usage of the same mode.
        if (mode != it->second.access_mode) {
            return ERR_VOLUME_ACCESS_DENIED;
        }
        // Reset the timer for this token
        LOGTRACE << "Renewing volume token: " << token;
        timer->cancel(it->second.timer_task);
        timer->schedule(it->second.timer_task, lease_time);
    }
    return ERR_OK;
}

void
DmVolumeAccessTable::removeToken(fds_int64_t const token) {
    std::unique_lock<std::mutex> lk(lock);
    auto it = access_map.find(token);
    if (access_map.end() != it) {
        timer->cancel(it->second.timer_task);
        if (cached && it->second.access_mode.can_cache) {
            cached = false;
        }
        access_map.erase(it);
    }
}

}  // namespace fds
