/* Copyright 2013 Formation Data Systems, Inc.
 */
#include "fds_counters.h"

#include <limits>
#include <sstream>
#include <ctime>
#include <chrono>
#include <sys/time.h>
#include <sys/resource.h>


namespace fds {

/* Snapshot all exported counters to snapshot_counters_*/
void SamplerTask::runTimerTask()
{
    fds_mutex::scoped_lock lock(lock_);
    for (auto counters : counters_ref_) {
        snapshot_counters_.push_back(new FdsCounters(*counters));
    }
}

/*****************************************************************************
 * Counter Manager
 ****************************************************************************/
FdsCountersMgr::FdsCountersMgr(const std::string &id)
    : id_(id),
      counters_lock_("Counters mutex"),
      timer_()
{
    defaultCounters = new FdsCounters("process", this);
    sampler_ptr_ = boost::shared_ptr<FdsTimerTask>(new SamplerTask(timer_, exp_counters_, snapshot_counters_));
    // bool ret = timer_.scheduleRepeated(sampler_ptr_, std::chrono::milliseconds(1000));
}

FdsCounters* FdsCountersMgr::get_default_counters() {
    return defaultCounters;
}

/**
 * @brief Adds FdsCounters object for export.
 *
 * @param counters - counters object to export.  Do not delete
 * counters before invoking remove_from_export
 */
void FdsCountersMgr::add_for_export(FdsCounters *counters)
{
    fds_mutex::scoped_lock lock(counters_lock_);
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
    fds_mutex::scoped_lock lock(counters_lock_);
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

    fds_mutex::scoped_lock lock(counters_lock_);

    std::time_t ts = std::time(NULL);

    for (auto counters : exp_counters_) {
        std::string counters_id = counters->id();
        for (auto c : counters->exp_counters_) {
            bool lat = typeid(*c) == typeid(LatencyCounter);
            std::string strId = lat ? c->id() + "." + std::to_string(c->volid().get()) +
                    ".latency" : c->id() + "." + std::to_string(c->volid().get());
            oss << id_ << "." << counters_id << "." << strId << " " << c->value() << " "
                    << ts << std::endl;
            if (lat) {
                strId = c->id() + "." + std::to_string(c->volid().get()) + ".count";
                oss << id_ << "." << counters_id << "." << strId << " " <<
                    dynamic_cast<LatencyCounter*>(c)->count()
                        << " " << ts << std::endl;
            }
        }
        counters->reset();
    }
    return oss.str();
}

/**
 * Exports to output stream
 * @param stream
 */
void FdsCountersMgr::export_to_ostream(std::ostream &stream)  // NOLINT
{
    fds_mutex::scoped_lock lock(counters_lock_);

    for (auto counters : exp_counters_) {
        std::string counters_id = counters->id();
        for (auto c : counters->exp_counters_) {
            bool lat = typeid(*c) == typeid(LatencyCounter);
            auto volString = std::to_string(c->volid().get());
            std::string strId = lat ? c->id() + "." + volString +
                    ".latency" : c->id() + "." + volString;
            stream << id_ << "." << counters_id << "." << strId << "\t\t" << c->value()
                    << std::endl;
            if (lat) {
                strId = c->id() + "." + volString + ".count";
                stream << id_ << "." << counters_id << "." << strId << "\t\t" <<
                        dynamic_cast<LatencyCounter*>(c)->count() << std::endl;
            }
        }
        counters->reset();
    }
}

/**
 * Converts to map
 * @param m
 */
void FdsCountersMgr::toMap(std::map<std::string, int64_t>& m)
{
    fds_mutex::scoped_lock lock(counters_lock_);

    for (auto counters : exp_counters_) {
        std::string counters_id = counters->id();
        for (auto c : counters->exp_counters_) {
            c->toMap(m);
        }
        counters->reset();
    }
}

/**
 * reset counters
 */
void FdsCountersMgr::reset()
{
    fds_mutex::scoped_lock lock(counters_lock_);
    for (auto counters : exp_counters_) {
        counters->reset();
    }
}

void FdsCountersMgr::add_for_export(StateProvider *provider)
{
    fds_mutex::scoped_lock lock(stateproviders_lock_);
    stateproviders_tbl_[provider->getStateProviderId()] = provider;
}

void FdsCountersMgr::remove_from_export(StateProvider *provider)
{
    fds_mutex::scoped_lock lock(stateproviders_lock_);
    stateproviders_tbl_.erase(provider->getStateProviderId());
}

bool FdsCountersMgr::getStateInfo(const std::string &id,
                                  std::map<std::string, std::string> &state)
{
    fds_mutex::scoped_lock lock(stateproviders_lock_);
    auto itr = stateproviders_tbl_.find(id);
    if (itr == stateproviders_tbl_.end()) {
        return false;
    }
    itr->second->getStateInfo(state);
    return true;
}


/*****************************************************************************
 * Counters
 ****************************************************************************/

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
 * Copy constructor
 */
FdsCounters::FdsCounters(const FdsCounters& counters) : id_(counters.id_)
{
    for (auto c : counters.exp_counters_) {
        bool lat = typeid(*c) == typeid(LatencyCounter);
        if (lat) {
            exp_counters_.push_back(new LatencyCounter(dynamic_cast<LatencyCounter&>(*c)));
        } else {
            exp_counters_.push_back(new NumericCounter(dynamic_cast<NumericCounter&>(*c)));
        }
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
        c->toMap(m);
    }
}

/**
 * Reset counters
 */
void FdsCounters::reset()  // NOLINT
{
    for (auto c : exp_counters_) {
        c->reset();
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

/*****************************************************************************
 * Base Counter
 ****************************************************************************/


/**
 * @brief  Base counter constructor.  Enables a counter to
 * be exported with an identifier.  If export_parent is NULL
 * counter will not be exported.
 *
 * Providing constructor with volume id enabled and without
 *
 * @param id - id to use when exporting the counter
 * @vol id - volume id (used for export)
 * @param export_parent - Pointer to the parent.  If null counter
 * is not exported.
 */

FdsBaseCounter::FdsBaseCounter(const std::string &id,
                                FdsCounters *export_parent)
: id_(id), volid_enable_(false), volid_(0)
{
    if (export_parent) {
        export_parent->add_for_export(this);
    }
}


FdsBaseCounter::FdsBaseCounter(const std::string &id, fds_volid_t volid,
                                FdsCounters *export_parent)
: id_(id), volid_enable_(true), volid_(volid)
{
    if (export_parent) {
        export_parent->add_for_export(this);
    }
}

/**
 * Copy Constructor
 */
FdsBaseCounter::FdsBaseCounter(const FdsBaseCounter &c)
: id_(c.id_), volid_enable_(c.volid_enable_), volid_(c.volid_)
{
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
 * Sets id
 * @param id
 */
void FdsBaseCounter::set_id(std::string id)
{
    id_ = id;
}

/**
 * Returns volid
 * @return
 */
fds_volid_t FdsBaseCounter::volid() const
{
    return volid_;
}

/**
 * Returns volid_enable
 * @return
 */
bool FdsBaseCounter::volid_enable() const
{
    return volid_enable_;
}

/**
 * Sets volid_
 */
void FdsBaseCounter::set_volid(fds_volid_t volid)
{
    volid_ = volid;
}

void FdsBaseCounter::toMap(std::map<std::string, int64_t>& m) const {
    auto volString = std::to_string(volid_.get());
    std::string strId = id() + (volid_enable()?"." + volString : "");
    m[strId] = static_cast<int64_t>(value());
}

/*****************************************************************************
 * Simple Numeric Counter - Cannot be reset via thrift
 ****************************************************************************/
SimpleNumericCounter::SimpleNumericCounter(const std::string &id, FdsCounters *export_parent) :
        FdsBaseCounter(id, export_parent), val_(0) {
}

SimpleNumericCounter::SimpleNumericCounter(const std::string &id, fds_volid_t volid,
                                           FdsCounters *export_parent) :
        FdsBaseCounter(id, volid, export_parent), val_(0) {
}

SimpleNumericCounter::SimpleNumericCounter(const SimpleNumericCounter& c) {
    val_ = c.val_.load(std::memory_order_relaxed);
}

uint64_t SimpleNumericCounter::value() const {
    return val_.load(std::memory_order_relaxed);
}

void SimpleNumericCounter::incr(const uint64_t v) {
    val_.fetch_add(v, std::memory_order_relaxed);
}

void SimpleNumericCounter::decr(const uint64_t v) {
    val_.fetch_sub(v, std::memory_order_relaxed);
}

void SimpleNumericCounter::set(const uint64_t v) {
    val_.store(v, std::memory_order_relaxed);
}


/*****************************************************************************
 * Numeric Counter
 ****************************************************************************/

/**
 * Constructor
 * @param id
 * @param export_parent
 */
NumericCounter::NumericCounter(const std::string &id, fds_volid_t volid,
                                FdsCounters *export_parent)
:   FdsBaseCounter(id, volid, export_parent)
{
    val_ = 0;
    min_value_ = std::numeric_limits<uint64_t>::max();
    max_value_ = 0;
}

NumericCounter::NumericCounter(const std::string &id, FdsCounters *export_parent)
: FdsBaseCounter(id, export_parent)
{
    val_ = 0;
    min_value_ = std::numeric_limits<uint64_t>::max();
    max_value_ = 0;
}
/**
 * Copy Constructor
 */
NumericCounter::NumericCounter(const NumericCounter& c)
: FdsBaseCounter(c)
{
    val_ = c.val_.load(std::memory_order_relaxed);
    min_value_ = c.min_value_.load(std::memory_order_relaxed);
    max_value_ = c.max_value_.load(std::memory_order_relaxed);
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
    return val_.load(std::memory_order_relaxed);
}

/**
 * reset counter
 */
void NumericCounter::reset()
{
    val_.store(0, std::memory_order_relaxed);
    min_value_.store(std::numeric_limits<uint64_t>::max(), std::memory_order_relaxed);
    max_value_.store(0, std::memory_order_relaxed);;
}
/**
 *
 */
void NumericCounter::incr() {
    incr(1);
}

/**
 *
 * @param v
 */
void NumericCounter::incr(const uint64_t v) {
    auto val = val_.fetch_add(v, std::memory_order_relaxed) + v;
    auto old_max = max_value_.load(std::memory_order_consume);
    if (old_max < val)
        { max_value_.store(val, std::memory_order_release); }
}

/**
 *
 */
void NumericCounter::decr() {
    decr(1);
}

/**
 *
 * @param v
 */
void NumericCounter::decr(const uint64_t v) {
    auto val = val_.fetch_sub(v, std::memory_order_relaxed) - v;
    auto old_min = min_value_.load(std::memory_order_consume);
    if (old_min > val)
        { min_value_.store(val, std::memory_order_release); }
}

/*****************************************************************************
 * Latency Counter
 ****************************************************************************/
LatencyCounter::LatencyCounter(const std::string &id, fds_volid_t volid,
                                FdsCounters *export_parent)
    :   FdsBaseCounter(id, volid, export_parent),
        total_latency_(0),
        cnt_(0),
        min_latency_(std::numeric_limits<uint64_t>::max()),
        max_latency_(0) {}

LatencyCounter::LatencyCounter(const std::string &id, FdsCounters *export_parent)
    :   FdsBaseCounter(id, export_parent),
        total_latency_(0),
        cnt_(0),
        min_latency_(std::numeric_limits<uint64_t>::max()),
        max_latency_(0) {}


/**
 * Copy Constructor
 */
LatencyCounter::LatencyCounter(const LatencyCounter &c)
    :   FdsBaseCounter(c),
        total_latency_(c.total_latency_.load()),
        cnt_(c.cnt_.load()),
        min_latency_(c.min_latency_.load()),
        max_latency_(c.max_latency_.load()) {}


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
 * reset counter
 */
void LatencyCounter::reset()
{
    total_latency_ = 0;
    cnt_ = 0;
    min_latency_ = std::numeric_limits<uint64_t>::max();
    max_latency_ = 0;
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

void LatencyCounter::toMap(std::map<std::string, int64_t>& m) const {
    m[id()] = static_cast<int64_t>(value());
    auto volString = std::to_string(volid().get());
    std::string strId = id() + (volid_enable()? "." + volString : "");
    m[strId + ".latency"] = static_cast<int64_t>(value());
    m[strId + ".count"] = static_cast<int64_t>(count());
}

/*****************************************************************************
 * ResourceUsage Counter
 ****************************************************************************/

ResourceUsageCounter::ResourceUsageCounter(FdsCounters *export_parent) : FdsBaseCounter("rusage", export_parent) {
}

void ResourceUsageCounter::toMap(std::map<std::string, int64_t>& m) const {
    struct rusage usage;
    getrusage (RUSAGE_SELF, &usage);
    m["rusage.usercpu.seconds"] = usage.ru_utime.tv_sec; /* user CPU time used */
    m["rusage.syscpu.seconds"] = usage.ru_stime.tv_sec; /* system CPU time used */
    m["rusage.mem.bytes"] = usage.ru_maxrss * 1024;        /* maximum resident set size */
    m["rusage.page.fault.soft"] = usage.ru_minflt;        /* page reclaims (soft page faults) */
    m["rusage.page.fault.hard"] = usage.ru_majflt;        /* page faults (hard page faults) */
    m["rusage.num.fs.input"] = usage.ru_inblock;       /* block input operations */
    m["rusage.num.fs.output"] = usage.ru_oublock;       /* block output operations */
    m["rusage.num.signals.rcvd"] = usage.ru_nsignals;      /* signals received */
    m["rusage.num.ctx.switch.voluntary"] = usage.ru_nvcsw;         /* voluntary context switches */
    m["rusage.num.ctx.switch.involuntary"] = usage.ru_nivcsw;        /* involuntary context switches */
}
}  // namespace fds
