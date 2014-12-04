/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_PROBE_FDS_ERR_INJ_H_
#define SOURCE_INCLUDE_FDS_PROBE_FDS_ERR_INJ_H_

#include <fds_types.h>

namespace fds {

typedef enum
{
    PR_POINT_NONE = 0,

    // Injection point ID for PM (Persistent Manager) is 0-0x200
    PR_PM_INJ_POINT_BASE = 0x1,
    PR_PM_INJ_POINT_END  = 0x1ff,

    // Injection point ID for SM (Storage Manager) is 0x200-0x800.
    PR_SM_INJ_POINT_BASE = 0x200,
    PR_SM_INJ_POINT_END  = 0x7ff
} probe_point_e;

typedef enum
{
    PR_ACT_NONE = 0
} probe_action_e;

// Associative pair to map id to string to identify injection points and
// actions.
//
typedef struct probe_inj_point probe_inj_point_t;
struct probe_inj_point
{
    probe_point_e            pr_inj_id;
    probe_action_e           pr_inj_act_id;
    char const *const        pr_inj_name;
};

typedef struct probe_inj_action probe_inj_action_t;
struct probe_inj_action
{
    // Specify the injection point ID that could attach to this action.
    probe_point_e            pr_inj_id;

    // True if this action can be applied to any injection points.
    fds_uint32_t             pr_act_global : 1;

    probe_action_e           pr_inj_act_id;
    char const *const        pr_inj_act_name;
};

// Stat name for a check-point and the time it took a request to complete it.
//
typedef struct probe_stat_info probe_stat_info_t;
struct probe_stat_info
{
    char const *const        pr_stat_name;
};

typedef struct probe_stat_rec probe_stat_rec_t;
struct probe_stat_rec
{
    char const *const        pr_stat_name;
    fds_uint64_t             pr_cputicks;
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_PROBE_FDS_ERR_INJ_H_
