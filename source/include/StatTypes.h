/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
/*
 * Systems stats types
 */
#ifndef SOURCE_INCLUDE_STATTYPES_H_
#define SOURCE_INCLUDE_STATTYPES_H_

#include "fds_typedefs.h" //NOLINT

namespace fds {

typedef enum {
    STAT_AM_GET_OBJ,
    STAT_AM_PUT_OBJ,

    STAT_MAX_TYPE  // last entry in the enum
} FdsStatType;

class FdsStatHash {
  public:
    fds_uint32_t operator()(FdsStatType evt) const {
        return static_cast<fds_uint32_t>(evt);
    }
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_STATTYPES_H_
