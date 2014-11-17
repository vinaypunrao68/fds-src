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

/**
 * Defaults (later could be configurable) for
 * sampling and pushing stats from individual modules to
 * an aggregator.
 */
#define FdsStatPeriodSec         60
#define FdsStatPushPeriodSec     180

typedef enum {
    STAT_AM_GET_OBJ,
    STAT_AM_GET_CACHED_OBJ,
    STAT_AM_PUT_OBJ,
    STAT_AM_GET_BMETA,
    STAT_AM_PUT_BMETA,
    STAT_AM_QUEUE_FULL,
    STAT_AM_QUEUE_WAIT,

    STAT_SM_GET_SSD,
    STAT_SM_CUR_DEDUP_BYTES,

    STAT_DM_BYTES_ADDED,
    STAT_DM_BYTES_REMOVED,
    STAT_DM_CUR_TOTAL_BYTES,
    STAT_DM_CUR_TOTAL_OBJECTS,
    STAT_DM_CUR_TOTAL_BLOBS,

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
