/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <FDSNStat.h>

namespace fds {

static const stat_decode_t stat_fdsn_decode[] =
{
    { FDSN_GO_CHK_BKET_EXIST,      "FDSN GetObj check bucket exists" },
    { FDSN_GO_ALLOC_BLOB_REQ,      "FDSN GetObj alloc blob req" },
    { FDSN_GO_ENQUEUE_IO,          "FDSN GetObj enqueue IO" },
    { FDSN_GO_ADD_WAIT_QUEUE,      "FDSN GetObj waitq OM" },
    { FDSN_GO_TOTAL,               "FDSN GetObj func" },
    { FDSN_MAX_STAT,               NULL }
};

// fdsn_register_stat
// ------------------
//
void
fdsn_register_stat(void)
{
    gl_fds_stat.stat_reg_mod(STAT_FDSN, stat_fdsn_decode);
}

} // namespace fds
