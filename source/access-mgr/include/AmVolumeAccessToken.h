/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMVOLUMEACCESSTOKEN_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMVOLUMEACCESSTOKEN_H_

#include <functional>

#include "fdsp/common_types.h"
#include "shared/fds_types.h"
#include "fds_timer.h"

namespace fds
{

struct AmVolumeAccessToken :
    public FdsTimerTask
{
    using mode_type = FDS_ProtocolInterface::VolumeAccessMode;
    using token_type = fds_int64_t;
    using callback_type = std::function<void()>;

    AmVolumeAccessToken(FdsTimer& _timer,
                        mode_type const& _mode,
                        token_type const _token,
                        callback_type&& _cb);
    AmVolumeAccessToken(AmVolumeAccessToken const&) = delete;
    AmVolumeAccessToken& operator=(AmVolumeAccessToken const&) = delete;
    ~AmVolumeAccessToken();

    void runTimerTask() override;

    bool cacheAllowed()
    { return mode.can_cache; }

    bool writeAllowed()
    { return mode.can_write; }

    mode_type getMode() const
    { return mode; }

    void setMode(mode_type const& _mode)
    { mode = _mode; }

    token_type getToken() const
    { return token; }

    void setToken(token_type const _token)
    { token = _token; }

  private:
    mode_type mode;
    token_type token;
    callback_type cb;
};

}  // namespace fds
#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMVOLUMEACCESSTOKEN_H_
