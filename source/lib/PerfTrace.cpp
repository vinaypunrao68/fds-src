#include <stdexcept>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#include "util/timeutils.h"
#include "fds_process.h"
#include "PerfTrace.h"

// XXX: uncomment if name space for context is limited
//#define CACHE_REGEX_MATCH_RESULTS

// XXX: uncomment this to use rdtsc()
//#define USE_RDTSC_TIME

namespace {

void createLatencyCounter(fds::PerfContext & ctx) {
    fds_assert(ctx.start_cycle);
    fds_assert(ctx.start_cycle <= ctx.end_cycle);

    fds::LatencyCounter * plc = new fds::LatencyCounter(ctx.name, 0);
    plc->update(ctx.end_cycle - ctx.start_cycle);
    ctx.data.reset(plc);
}

template <typename T>
void initializeCounter(fds::PerfContext * ctx, fds::FdsCounters * parent) {
    fds_assert(ctx);
    fds_assert(0 == ctx->data.get());
    GLOGDEBUG << "Creating performance counter for type='" << ctx->type
            << "' name='" << ctx->name << "'";

    std::string counterName = fds::eventTypeToStr[ctx->type];
    if (!ctx->name.empty()) {
        counterName += "." + ctx->name;
    }

    ctx->data.reset(new T(counterName, parent));
}

void stringToEventsFilter(const std::string & str, std::bitset<fds::MAX_EVENT_TYPE> & filter) {
    // Sanitize the input and return bitset
    std::vector<std::string> tokenVec;
    boost::split(tokenVec, str, boost::is_any_of(","));
    for(std::string & str : tokenVec) {
        boost::trim(str);
        unsigned startPos = fds::MAX_EVENT_TYPE;
        unsigned endPos = startPos;
        if (std::string::npos == str.find('-')) {
            startPos = endPos = boost::lexical_cast<unsigned>(str);
        } else {

            std::vector<std::string> rangeVec;
            boost::split(rangeVec, str, boost::is_any_of("-"));
            if (2 != rangeVec.size()) {
                throw std::runtime_error("Invalid event filter string");
            }

            boost::trim(rangeVec[0]);
            startPos = boost::lexical_cast<unsigned>(rangeVec[0]);
            boost::trim(rangeVec[1]);
            endPos = boost::lexical_cast<unsigned>(rangeVec[1]);
        }

        if (fds::MAX_EVENT_TYPE <= startPos) {
            throw std::runtime_error("Invalid start value in a range");
        }
        if (endPos < startPos) {
            throw std::runtime_error("Invalid end value in a range");
        }

        if (fds::MAX_EVENT_TYPE <= endPos) {
            endPos = fds::MAX_EVENT_TYPE - 1;
        }

        for(unsigned pos = startPos; pos <= endPos; ++pos) {
            filter[pos] = 1;
        }
    }
}

} /* namespace (anonymous) */

namespace fds {

const std::string PERF_COUNTERS_NAME("perf");

const char * eventTypeToStr[] = {
        "trace", //generic event

        "qos",
        "murmur3_hash",
        "dlt_lkup",
        "dmt_lkup",
        "put_obj",
        "get_obj",
    
        "put_obj_dedupe_chk",
        "persist_disk_write",
        "put_obj_loc_indx_update",

        "get_obj_cache_lkup",
        "get_obj_lkup_loc_indx",
        "get_obj_pl_read_disk"
};

PerfTracer::PerfTracer() : aggregateCounters_(fds::MAX_EVENT_TYPE),
                           namedCounters_(fds::MAX_EVENT_TYPE),
                           enable_(true),
                           useEventsFilter_(false),
                           nameFilter_("^$"),
                           useNameFilter_(false),
                           config_helper_(g_fdsprocess->get_conf_helper()) {
    fds_assert(g_cntrs_mgr);
    fds_assert(g_fdslog);

    GLOGDEBUG << "Instantiating PerfTracer";
    for (unsigned i = 0; i < MAX_EVENT_TYPE; ++i) {
        aggregateCounters_[i].type = static_cast<PerfEventType>(i);
        aggregateCounters_[i].name = "";
    }

    exportedCounters.reset(new FdsCounters(PERF_COUNTERS_NAME, g_cntrs_mgr.get()));

    reconfig();
}

PerfTracer::~PerfTracer() {
    GLOGDEBUG << "Destroying PerfTracer";
    for (auto& kv : latencyMap_) {
        delete kv.second;
    }
    latencyMap_.clear();

    for (PerfContextMap & m : namedCounters_) {
        for (auto& kv : m) {
            //TODO: exportedCounters->remove_from_export(kv.second.data);
            delete kv.second;
        }
        m.clear();
    }
    namedCounters_.clear();
}

PerfTracer & PerfTracer::instance() {
    static PerfTracer instance_;
    return instance_;
}

void PerfTracer::reconfig() {
    GLOGDEBUG << "Reading PerfTrace configuration";

    // read global enable/ disable settings
    bool tmpEnable = config_helper_.get<bool>(PERF_COUNTERS_NAME + "." + "enable", false);

    // read events filter settings
    const std::string eventsFilterKey(PERF_COUNTERS_NAME + "." + "event_types");
    std::bitset<MAX_EVENT_TYPE> efilt;
    bool tmpUseEventsFilter = config_helper_.exists(eventsFilterKey);
    if (tmpUseEventsFilter) {
        std::string efStr = config_helper_.get<std::string>(eventsFilterKey);

       // process events filter settings
       try {
           stringToEventsFilter(efStr, efilt);
       } catch (std::exception & e) {
           GLOGWARN << "Error while reading events filter, ignoring the configuration. "
                   << e.what();
           tmpUseEventsFilter = false;
       }
    }

    // read name filter settings
    const std::string nameFilterKey(PERF_COUNTERS_NAME + "." + "match");
    std::string exprStr;
    bool tmpUseNameFilter = config_helper_.exists(nameFilterKey);
    if (tmpUseNameFilter) {
        exprStr = config_helper_.get<std::string>(nameFilterKey);
    }
    if(exprStr.empty()) {
        tmpUseNameFilter = false;
    }

    FDSGUARD(config_mutex_);

    // update name filter config
    if (tmpUseNameFilter) {
        try {
            nameFilter_ = exprStr;
        } catch (boost::bad_expression & ex) {
            GLOGWARN << "Invalid expression in name filter, ignoring configuration";
            tmpUseNameFilter = false;
        }
    }
    useNameFilter_ = tmpUseNameFilter;

    // update events filter config
    if (tmpUseEventsFilter) {
        for (unsigned i = 0; i < MAX_EVENT_TYPE; ++i) {
            eventsFilter_[i] = efilt[i];
        }
    }
    useEventsFilter_ = tmpUseEventsFilter;

    // update global enable/ disable
    enable_ = tmpEnable;
}

void PerfTracer::updateCounter(PerfContext & ctx, const fds_uint64_t & val,
        const fds_uint64_t cnt) {
    GLOGNORMAL << "Updating performance counter for type='" << ctx.type
            << "' val='" << val << "' count='" << cnt << "' name='"
            << ctx.name << "'";
    FdsCounters * counterParent = ctx.enabled ? exportedCounters.get() : 0;
    // update counter
    if (cnt) {
        std::call_once(*ctx.once, initializeCounter<LatencyCounter>, &ctx, counterParent);
        LatencyCounter * plc = dynamic_cast<LatencyCounter *>(ctx.data.get());
        fds_assert(plc || !"Counter type mismatch between tracepoints!")
        plc->update(val, cnt);
    } else {
        std::call_once(*ctx.once, initializeCounter<NumericCounter>, &ctx, counterParent);
        NumericCounter * pnc = dynamic_cast<NumericCounter *>(ctx.data.get());
        fds_assert(pnc || !"Counter type mismatch between tracepoints!")
        pnc->incr(val);
    }
}

void PerfTracer::upsert(const PerfEventType & type, fds_uint64_t val, fds_uint64_t cnt,
            const std::string & name) {
    PerfContext * ctx = 0;

    FDSGUARD(ptrace_mutex_);

    PerfContextMap::iterator pos = namedCounters_[type].find(name);
    if (namedCounters_[type].end() != pos) {
        if (!pos->second->enabled) {
            return; // disabled for this name
        }

        ctx = pos->second;
    } else {
        ctx = new PerfContext(type, name);
        if (!name.empty()) {
#ifdef CACHE_REGEX_MATCH_RESULTS
            if (!regex_match(name, nameFilter_)) {
                ctx->enabled = false;
            }
#endif
        }

        namedCounters_[type][name] = ctx;
    }

    // update counter
    updateCounter(*ctx, val, cnt);
}

void PerfTracer::incr(const PerfEventType & type, std::string name /* = "" */) {
    PerfTracer::incr(type, 1, 0, name);
}

void PerfTracer::incr(const PerfEventType & type, fds_uint64_t val,
        fds_uint64_t cnt /* = 0 */, std::string name /* = "" */) {
    fds_assert(type < MAX_EVENT_TYPE);

    // check if performance data collection is enabled
    if (!instance().enable_) {
        return;
    }

    if (instance().useEventsFilter_  && !instance().eventsFilter_[type]) {
        return;
    }

    // update aggregate counter first
    instance().updateCounter(instance().aggregateCounters_[type], val, cnt);

    if (!name.empty() && instance().useNameFilter_) {
#ifndef CACHE_REGEX_MATCH_RESULTS
        if (regex_match(name, instance().nameFilter_)) {
#endif
            instance().upsert(type, val, cnt, name);
#ifndef CACHE_REGEX_MATCH_RESULTS
        }
#endif
    }
}

void PerfTracer::tracePointBegin(const std::string & id, const PerfEventType & type,
            std::string name /* = "" */) {
    GLOGDEBUG << "Received tracePointBegin() for id='" << id << "' type='" <<
            type << "' name='" << name << "'";
    PerfContext * ctx = new PerfContext(type, name);
    tracePointBegin(*ctx);

    FDSGUARD(instance().latency_map_mutex_);
#ifdef DEBUG
    PerfContextMap::iterator pos = instance().latencyMap_.find(id);
    fds_assert(instance().latencyMap_.end() == pos);
#endif

    instance().latencyMap_[id] = ctx;
}

void PerfTracer::tracePointBegin(PerfContext & ctx) {
    GLOGDEBUG << "Starting perf trace for type='" << ctx.type << "' name='" <<
            ctx.name << "'";
#ifdef USE_RDTSC_TIME
    ctx.start_cycle = util::getNanosFromTicks(util::getClockTicks());
#else
    ctx.start_cycle = util::getTimeStampNanos();
#endif
}

boost::shared_ptr<PerfContext> PerfTracer::tracePointEnd(const std::string & id) {
    PerfContext * ppc = 0;

    GLOGDEBUG << "Received tracePointEnd() for id='" << id << "'";
    {
        FDSGUARD(instance().latency_map_mutex_);

        PerfContextMap::iterator pos = instance().latencyMap_.find(id);
        fds_assert(instance().latencyMap_.end() != pos);

        ppc = pos->second;
        instance().latencyMap_.erase(pos);
    }

    tracePointEnd(*ppc);

    return boost::shared_ptr<PerfContext>(ppc);
}

void PerfTracer::tracePointEnd(PerfContext & ctx) {
    GLOGDEBUG << "Ending perf trace for type='" << ctx.type << "' name='" <<
            ctx.name << "'";
#ifdef USE_RDTSC_TIME
    ctx.end_cycle = util::getNanosFromTicks(util::getClockTicks());
#else
    ctx.end_cycle = util::getTimeStampNanos();
#endif
    createLatencyCounter(ctx);
    LatencyCounter * plc = dynamic_cast<LatencyCounter *>(ctx.data.get());
    incr(ctx.type, plc->total_latency(), plc->count(), ctx.name);
}

void PerfTracer::setEnabled(bool val /* = true */) {
    // FDSGUARD(instance().config_mutex_);
    instance().enable_ = val;
}

bool PerfTracer::isEnabled() {
    return instance().enable_;
}

void PerfTracer::refresh() {
    instance().reconfig();
}

} /* namespace fds */
