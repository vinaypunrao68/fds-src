/*
 * Copyright 2013-2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_COMMON_H_
#define SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_COMMON_H_

// Forward declare so we can hide the ev++.h include
// in the cpp file so that it doesn't conflict with
// the libevent headers in Thrift.
namespace ev {
class io;
class async;
}  // namespace ev

namespace fds
{

    enum class ScstError : uint8_t {
        connection_closed,
        shutdown_requested,
    };

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_COMMON_H_
