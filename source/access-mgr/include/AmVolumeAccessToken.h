/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMVOLUMEACCESSTOKEN_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMVOLUMEACCESSTOKEN_H_

#include <functional>

#include "shared/fds_types.h"
#include "fds_timer.h"

namespace fds
{

struct AmVolumeAccessToken : public FdsTimerTask
{
    using token_type = fds_int64_t;
    using callback_type = std::function<void(token_type const)>;

    AmVolumeAccessToken(FdsTimer& _timer, token_type const _token, callback_type&& _cb);
    ~AmVolumeAccessToken() = default;

    void runTimerTask() override;
    token_type getToken() const;

  private:
   token_type token;
   callback_type cb;
};

}  // namespace fds
#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMVOLUMEACCESSTOKEN_H_
