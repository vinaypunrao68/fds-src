/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include "AmVolumeAccessToken.h"
#include "util/Log.h"

namespace fds
{

AmVolumeAccessToken::AmVolumeAccessToken(FdsTimer& _timer,
                                         token_type const _token,
                                         callback_type&& _cb)
    : FdsTimerTask(_timer),
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
