/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_UTIL_HASH_UTIL_H_
#define SOURCE_INCLUDE_UTIL_HASH_UTIL_H_

#include <shared/fds_types.h>

/*
 * Hash keys utility
 */
namespace fds {

struct UuidIntKey
{
    fds_uint64_t             h_key;
    fds_uint32_t             h_int1;
    fds_uint32_t             h_int2;

    bool operator == (const UuidIntKey &rhs) const {
        return ((this->h_key == rhs.h_key) &&
                (this->h_int1 == rhs.h_int1) && (this->h_int2 == rhs.h_int2));
    }
    bool operator != (const UuidIntKey &rhs) const {
        return !(*this == rhs);
    }
};

struct UuidIntKeyHash {
    fds_uint64_t operator()(const UuidIntKey &v) const {
        return v.h_key + v.h_int1 + v.h_int2;
    }
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_UTIL_HASH_UTIL_H_
