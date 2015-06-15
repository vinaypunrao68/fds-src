/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include "AmVolumeAccessToken.h"
#include "util/Log.h"

namespace fds
{

AmVolumeAccessToken::AmVolumeAccessToken(FdsTimer& _timer,
                                         mode_type const& _mode,
                                         token_type const _token,
                                         callback_type&& _cb)
    : FdsTimerTask(_timer),
      mode(_mode),
      token(_token),
      cb(std::move(_cb))
{
}

AmVolumeAccessToken::~AmVolumeAccessToken() {
    LOGTRACE << "AccessToken for: 0x" << std::hex << token << " is being destroyed";
}

void
AmVolumeAccessToken::runTimerTask() {
    LOGTRACE << "AccessToken for: 0x" << std::hex << token << " is being renewed";
    if (cb) {
        cb();
    }
}

}  // namespace fds
