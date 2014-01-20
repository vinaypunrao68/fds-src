/* Copyright 2013 Formation Data Systems, Inc.
 */
#include <string>
#include <fds_counters.h>

namespace fds {

/**
 * @brief 
 *
 * @param id
 * @param counters
 */
void FdsCountersMgr::add_for_export(FdsCounters *counters)
{
}

/**
 * @brief 
 *
 * @param counters
 */
void FdsCountersMgr::remove_from_export(FdsCounters *counters)
{
}

std::string FdsCountersMgr::export_as_graphite()
{
    return "";
}

}  // namespace fds
