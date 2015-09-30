/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_COMMON_H_
#define SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_COMMON_H_

// Forward declare so we can hide the ev++.h include
// in the cpp file so that it doesn't conflict with
// the libevent headers in Thrift.
namespace ev {
class io;
class async;
class dynamic_loop;
}  // namespace ev

namespace fds
{

    enum class ScstError : uint8_t {
        scst_not_found,
        scst_error,
    };

}  // namespace fds

#define READ_CAPACITY_16    0x9E

#endif  // SOURCE_ACCESS_MGR_INCLUDE_CONNECTOR_SCST_COMMON_H_
