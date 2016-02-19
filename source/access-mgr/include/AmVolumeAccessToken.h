/*
 * Copyright 2015-2016 Formation Data Systems, Inc.
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

    AmVolumeAccessToken() :
        mode(), token(invalid_vol_token)
    { mode.can_write = false; mode.can_cache = false; }

    AmVolumeAccessToken(mode_type const& _mode, token_type const _token) :
        mode(_mode), token(_token)
    { }

    ~AmVolumeAccessToken() = default;

    bool cacheAllowed() const
    { return mode.can_cache; }

    bool writeAllowed() const
    { return mode.can_write; }

    token_type getToken() const
    { return token; }

  private:
    mode_type mode;
    token_type token;
};

}  // namespace fds
#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMVOLUMEACCESSTOKEN_H_
