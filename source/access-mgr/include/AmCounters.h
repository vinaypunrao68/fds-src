/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_AMCOUNTERS_H_
#define SOURCE_ACCESS_MGR_INCLUDE_AMCOUNTERS_H_

#include <string>

#include "fds_counters.h"

class AMCounters : public fds::FdsCounters
{
 public:
    AMCounters(const std::string &id, fds::FdsCountersMgr *mgr)
    : fds::FdsCounters(id, mgr),
      put_reqs("put_reqs", this),
      get_reqs("get_reqs", this),
      puts_latency("puts_latency", this),
      gets_latency("gets_latency", this)
    {
    }

    fds::NumericCounter put_reqs;
    fds::NumericCounter get_reqs;
    fds::LatencyCounter puts_latency;
    fds::LatencyCounter gets_latency;
};

#endif  // SOURCE_ACCESS_MGR_INCLUDE_AMCOUNTERS_H_
