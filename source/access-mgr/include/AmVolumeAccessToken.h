/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMVOLUMEACCESSTOKEN_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMVOLUMEACCESSTOKEN_H_

#include <functional>

#include "fdsp/common_types.h"
#include "shared/fds_types.h"

namespace fds
{

struct AmVolumeAccessToken
{
    using mode_type = FDS_ProtocolInterface::VolumeAccessMode;
    using token_type = fds_int64_t;

    AmVolumeAccessToken(mode_type const& _mode, token_type const _token) :
        mode(_mode), token(_token)
    { }

    AmVolumeAccessToken(AmVolumeAccessToken const&) = delete;
    AmVolumeAccessToken& operator=(AmVolumeAccessToken const&) = delete;
    ~AmVolumeAccessToken() = default;

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
};

}  // namespace fds
#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMVOLUMEACCESSTOKEN_H_
