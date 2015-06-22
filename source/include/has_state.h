/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_HAS_STATE_H_
#define SOURCE_INCLUDE_HAS_STATE_H_

#include <string>

#include "fdsp/common_types.h"

namespace fds
{

struct HasState {
    virtual FDS_ProtocolInterface::ResourceState getState() const = 0;
    virtual void setState(FDS_ProtocolInterface::ResourceState state) = 0;

    std::string getStateName() const;
    bool isStateLoading() const;
    bool isStateCreated() const;
    bool isStateActive() const;
    bool isStateOffline() const;
    bool isStateMarkedForDeletion() const;
    bool isStateDeleted() const;
    virtual ~HasState() {}
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_HAS_STATE_H_
