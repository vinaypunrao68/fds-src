/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <platform/platform-lib.h>
#include <platform/domain_resources.h>

namespace fds
{
    DomainResources::~DomainResources()
    {
    }

    DomainResources::DomainResources(char const *const name) : drs_dlt(NULL), rs_refcnt(0)
    {
        drs_cur_throttle_lvl = 0;
        drs_dmt_version      = 0;
    }
}  // namespace fds
