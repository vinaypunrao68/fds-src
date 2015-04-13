/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include "AmVolumeAccessToken.h"

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

void 
AmVolumeAccessToken::runTimerTask() {
    cb(token);
}

AmVolumeAccessToken::token_type
AmVolumeAccessToken::getToken() const {
    return token;
}

}  // namespace fds
