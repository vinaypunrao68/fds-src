/* Copyright 2013 Formation Data Systems, Inc.
 */
#include <string>
#include <sstream>
#include <fds_counters.h>
#include <ctime>
namespace fds {

FdsCountersMgr::FdsCountersMgr(const std::string &id)
    : id_(id),
      lock_("Counters mutex")
{
}
/**
 * @brief Adds FdsCounters object for export.  
 *
 * @param counters - counters object to export.  Do not delete
 * counters before invoking remove_from_export 
 */
void FdsCountersMgr::add_for_export(FdsCounters *counters)
{
    fds_mutex::scoped_lock lock(lock_);
    exp_counters_.push_back(counters);
}

/**
 * @brief 
 *
 * @param counters
 */
void FdsCountersMgr::remove_from_export(FdsCounters *counters)
{
    fds_verify(!"Not implemented yet");
}

/**
 * @brief Constructs graphite string out of the counters objects registered for
 * export
 *
 * @return 
 */
std::string FdsCountersMgr::export_as_graphite()
{
    std::ostringstream oss;

    fds_mutex::scoped_lock lock(lock_);

    std::time_t ts = std::time(NULL);

    for (auto counters : exp_counters_) {
        std::string counters_id = counters->id();
        for (auto c : counters->exp_counters_) {
            oss << id_ << "." << counters_id << "." << c->id() << " " << c->value()
                << " " << ts << std::endl;
        }
    }
    return oss.str();
}

}  // namespace fds
