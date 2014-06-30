/* Copyright 2013 Formation Data Systems, Inc.
 */
#include <limits>
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
 * Returns counters identified by id
 *
 * @param id
 * @return
 */
FdsCounters* FdsCountersMgr::get_counters(const std::string &id)
{
    fds_mutex::scoped_lock lock(lock_);
    /* Iterate till we find the counters we care about.
     * NOTE: When we have a lot of exported counters this is
     * inefficient.  For now this should be fine.
     */
    for (auto c : exp_counters_) {
        if (c->id() == id) {
            return c;
        }
    }
    return nullptr;
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
            bool lat = typeid(*c) == typeid(LatencyCounter);
            std::string strId = lat ? c->id() + ".latency" : c->id();
            oss << id_ << "." << counters_id << "." << strId << " " << c->value() << " "
                    << ts << std::endl;
            if (lat) {
                strId = c->id() + ".count";
                oss << id_ << "." << counters_id << "." << strId << " " <<
                    dynamic_cast<LatencyCounter*>(c)->count()
                        << " " << ts << std::endl;
            }
        }
    }
    return oss.str();
}

/**
 * Exports to output stream
 * @param stream
 */
void FdsCountersMgr::export_to_ostream(std::ostream &stream)  // NOLINT
{
    fds_mutex::scoped_lock lock(lock_);

    for (auto counters : exp_counters_) {
        std::string counters_id = counters->id();
        for (auto c : counters->exp_counters_) {
            bool lat = typeid(*c) == typeid(LatencyCounter);
            std::string strId = lat ? c->id() + ".latency" : c->id();
            stream << id_ << "." << counters_id << "." << strId << ":\t\t" << c->value()
                    << std::endl;
            if (lat) {
                strId = c->id() + ".count";
                stream << id_ << "." << counters_id << "." << strId << ":\t\t" <<
                        dynamic_cast<LatencyCounter*>(c)->count() << std::endl;
            }
        }
    }
}

/**
 * Converts to map
 * @param m
 */
void FdsCountersMgr::toMap(std::map<std::string, int64_t>& m)
{
    fds_mutex::scoped_lock lock(lock_);

    for (auto counters : exp_counters_) {
        std::string counters_id = counters->id();
        for (auto c : counters->exp_counters_) {
            bool lat = typeid(*c) == typeid(LatencyCounter);
            std::string strId = lat ? c->id() + ".latency" : c->id();
            m[strId] = static_cast<int64_t>(c->value());
            if (lat) {
                strId = c->id() + ".count";
                m[strId] = static_cast<int64_t>(
                        dynamic_cast<LatencyCounter*>(c)->count());
            }
        }
    }
}

/**
 * Constructor
 * @param id
 * @param mgr
 */
FdsCounters::FdsCounters(const std::string &id, FdsCountersMgr *mgr)
: id_(id)
{
    if (mgr) {
        mgr->add_for_export(this);
    }
}

/**
 * Exposed for mock testing
 */
FdsCounters::FdsCounters() {}

/**
 * Destructor
 */
FdsCounters::~FdsCounters() {}

/**
 * Returns id
 * @return
 */
std::string FdsCounters::id() const
{
    return id_;
}

/**
 * Converts to string
 * @return
 */
std::string FdsCounters::toString()
{
    std::ostringstream oss;
    for (auto c : exp_counters_) {
        bool lat = typeid(*c) == typeid(LatencyCounter);
        std::string strId = lat ? c->id() + ".latency" : c->id();
        oss << id_ << "." << "." << strId << " = " << c->value() << "; ";
        if (lat) {
            strId = c->id() + ".count";
            oss << id_ << "." << "." << strId << " = " <<
                dynamic_cast<LatencyCounter*>(c)->count() << "; ";
        }
    }
    return oss.str();
}

/**
 * Converts to map
 * @param m
 */
void FdsCounters::toMap(std::map<std::string, int64_t>& m) const  // NOLINT
{
    for (auto c : exp_counters_) {
        bool lat = typeid(*c) == typeid(LatencyCounter);
        std::string strId = lat ? c->id() + ".latency" : c->id();
        m[strId] = static_cast<int64_t>(c->value());
        if (lat) {
            strId = c->id() + ".count";
            m[strId] = static_cast<int64_t>(
                    dynamic_cast<LatencyCounter*>(c)->count());
        }
    }
}

/**
 * @brief Marks the counter for export.  Only export counters
 * that are members of the derived class.  This method is invoked
 * by FdsBaseCounter constructor to mark the counter as exported.
 *
 * @param cp Counter to export
 */
void FdsCounters::add_for_export(FdsBaseCounter* cp)
{
    fds_assert(cp);
    exp_counters_.push_back(cp);
}

void FdsCounters::remove_from_export(FdsBaseCounter* cp)
{
    fds_verify(!"Not implemented yet");
}

/**
 * @brief  Base counter constructor.  Enables a counter to
 * be exported with an identifier.  If export_parent is NULL
 * counter will not be exported.
 *
 * @param id - id to use when exporting the counter
 * @param export_parent - Pointer to the parent.  If null counter
 * is not exported.
 */
FdsBaseCounter::FdsBaseCounter(const std::string &id, FdsCounters *export_parent)
: id_(id)
{
    if (export_parent) {
        export_parent->add_for_export(this);
    }
}
/**
 * Exposed for mock testing
 */
FdsBaseCounter::FdsBaseCounter() {}

/**
 * Destructor
 */
FdsBaseCounter::~FdsBaseCounter()
{
}

/**
 * Returns id
 * @return
 */
std::string FdsBaseCounter::id() const
{
    return id_;
}

/**
 * Constructor
 * @param id
 * @param export_parent
 */
NumericCounter::NumericCounter(const std::string &id, FdsCounters *export_parent)
: FdsBaseCounter(id, export_parent)
{
    val_ = 0;
}
/**
 *  Exposed for testing
 */
NumericCounter::NumericCounter() {}
/**
 *
 * @return
 */
uint64_t NumericCounter::value() const
{
    return val_.load();
}
/**
 *
 */
void NumericCounter::incr() {
    val_++;
}

/**
 *
 * @param v
 */
void NumericCounter::incr(const uint64_t v) {
    val_ += v;
}
/**
 *
 * @param id
 * @param export_parent
 */
LatencyCounter::LatencyCounter(const std::string &id, FdsCounters *export_parent)
    :   FdsBaseCounter(id, export_parent),
        total_latency_(0),
        cnt_(0),
        min_latency_(std::numeric_limits<uint64_t>::max()),
        max_latency_(0) {}
/**
 * Exposed for testing
 */
LatencyCounter::LatencyCounter()
    :   total_latency_(0),
        cnt_(0),
        min_latency_(std::numeric_limits<uint64_t>::max()),
        max_latency_(0) {}

/**
 *
 * @return
 */
uint64_t LatencyCounter::value() const
{
    uint64_t cnt = count();
    if (cnt == 0) {
        return 0;
    }
    return total_latency() / cnt;
}
/**
 *
 * @param val
 * @param cnt
 */
void LatencyCounter::update(const uint64_t &val, uint64_t cnt /* = 1 */) {
    total_latency_.fetch_add(val);
    cnt_.fetch_add(cnt);
    if (1 == cnt) {
        if (val < min_latency()) {
            min_latency_.store(val);
        }
        if (val > max_latency()) {
            max_latency_.store(val);
        }
    }
}

LatencyCounter & LatencyCounter::operator +=(const LatencyCounter & rhs) {
    if (&rhs != this) {
        update(rhs.total_latency(), rhs.count());

        uint64_t rhs_min = rhs.min_latency();
        if (rhs_min < min_latency()) {
            min_latency_.store(rhs_min);
        }

        uint64_t rhs_max = rhs.max_latency();
        if (rhs_max > max_latency()) {
            max_latency_.store(rhs_max);
        }
    }
    return *this;
}

}  // namespace fds
