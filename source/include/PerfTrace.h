#ifndef SOURCE_PERF_TRACE_H_
#define SOURCE_PERF_TRACE_H_

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>

#include <utility>
#include <vector>
#include <unordered_map>
#include <bitset>
#include <thread>
#include <mutex>

#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>

#include "fds_counters.h"
#include "fds_config.hpp"
#include "util/Log.h"
#include "concurrency/Mutex.h"
#include "PerfTypes.h"

#define SCOPED_PERF_TRACEPOINT(id, type, volid, name) fds::ScopedTracePoint stp_(id, type, volid, name)
#define SCOPED_PERF_TRACEPOINT_CTX(ctx) fds::ScopedTracePointCtx stpctx_(ctx)

#ifdef DEBUG

#define PERF_TRACEPOINT_INCR(type, volid, name) fds::PerfTracer::incr(type, volid, name)
#define PERF_TRACEPOINT_BEGIN(id, type, volid, name) fds::PerfTracer::tracePointBegin(id, type, volid, name)
#define PERF_TRACEPOINT_BEGIN_CTX(ctx) fds::PerfTracer::tracePointBegin(ctx)
#define PERF_TRACEPOINT_END_CTX(ctx) fds::PerfTracer::tracePointEnd(ctx)
#define PERF_TRACEPOINT_END(id, volid) fds::PerfTracer::tracePointEnd(id, volid)

#define SCOPED_PERF_TRACEPOINT_DEBUG(id, type, volid, name) fds::ScopedTracePoint stp_(id, type, volid, name)
#define SCOPED_PERF_TRACEPOINT_CTX_DEBUG(ctx) fds::ScopedTracePointCtx stpctx_(ctx)

#else

#define PERF_TRACEPOINT_INCR(type, name)
#define PERF_TRACEPOINT_BEGIN(id, type, name)
#define PERF_TRACEPOINT_BEGIN_CTX(ctx)
#define PERF_TRACEPOINT_END(idOrCtx)

#define SCOPED_PERF_TRACEPOINT_DEBUG(id, type, name)
#define SCOPED_PERF_TRACEPOINT_CTX_DEBUG(ctx)

#endif /* DEBUG */

namespace fds { 

extern const std::string PERF_COUNTERS_NAME;

/**
 * Per module (SM/DM/SH) performance tracer
 *
 * This class is a sigleton and provides static utility functions for performance
 * data collection. It supports LatencyCounters and NumericCounters.
 */
class PerfTracer : public boost::noncopyable {
public:

    // Increment NumericCounter by 1
    static void incr(const PerfEventType & type, fds_volid_t volid, std::string const& name = "");

    // Increment counter value by val and (for LatencyCounter, count by cnt)
    static void incr(const PerfEventType & type, fds_volid_t volid, uint64_t val,
            uint64_t cnt = 0, std::string const& name = "");

    // Decrement NumericCounter by 1
    static void decr(const PerfEventType & type, fds_volid_t volid, std::string name = "");

    // Decrement NumericCounter value by val
    static void decr(const PerfEventType & type, fds_volid_t volid, uint64_t val,
            std::string name = "");


    // For LatencyCounters
    /**
     * Begins trace point for calculating latency.
     *
     * Call this function typically at the start of some operation. This will get
     * the current timestamp in nanoseconds and store it for given id. This will be
     * used with tracePointEnd().
     *
     * @arg id: Used only to correlate tracePointBegin() and tracePointEnd()
     * @arg type: type of the event
     * @arg name: default is empty (ignored). If specified, LatencyCounter will be
     *            maintained per type, per name.
     */
    // update begin timestamp for identified event
    static void tracePointBegin(const std::string & id, const PerfEventType & type,
            fds_volid_t volid, std::string name = "");
    /**
     * Begins trace point for calculating latency.
     *
     * Faster version as its lock free and no id based lookup is done.
     */
    static void tracePointBegin(PerfContext & ctx);

    // update end timestamp for identified context and updates latency
    static boost::shared_ptr<PerfContext> tracePointEnd(const std::string & id);

    static void tracePointEnd(PerfContext & ctx);

    // enable/ disable performance data collection
    static void setEnabled(bool val = true);
    static bool isEnabled();

    // reload the configuration
    static void refresh();

private:

    typedef std::unordered_map<std::string, PerfContext *> PerfContextMap;
    PerfContextMap latencyMap_;
    fds_mutex latency_map_mutex_; //only for latencyMap_

    // all the counters
    boost::shared_ptr<FdsCounters> exportedCounters;

    std::vector<std::unordered_map<fds_volid_t, PerfContext>> aggregateCounters_;
    fds_mutex ptrace_mutex_aggregate_;   // only for aggregate

    std::vector<std::unordered_map<fds_volid_t, PerfContextMap>> namedCounters_;
    fds_mutex ptrace_mutex_named_; // only for namedCounters_

    fds_mutex ptrace_mutex_; 

    /*
     * configuration
     */
    // enable/ disable performace data collection
    bool enable_;

    // events filter for event type
    std::bitset<fds_enum::get_size<PerfEventType>()> eventsFilter_;
    bool useEventsFilter_;

    // events filter for event name
    boost::regex nameFilter_;
    bool useNameFilter_;

    // as the configuration will not be refreshed frequently,
    // we don't want penalty during reads
    FdsConfigAccessor config_helper_;
    fds_mutex config_mutex_; // only for writes, NOT for reads

    /*
     * Methods
     */
    PerfTracer();
    ~PerfTracer();

    void reconfig();

    void updateCounter(PerfContext & ctx, const PerfEventType & type, 
                const uint64_t & val, const uint64_t cnt,  const fds_volid_t volid, 
                const std::string& name);

    // update & insert (if counter is not present)
    void upsert(const PerfEventType & type, fds_volid_t volid, uint64_t val, 
            uint64_t cnt, const std::string & name);

    void decrement(const PerfEventType & type, fds_volid_t volid, 
            uint64_t val, const std::string & name);
    static inline PerfTracer & instance();
};

typedef struct ScopedTracePoint_ {
    ScopedTracePoint_(const std::string & id, const PerfEventType & type, 
            fds_volid_t volid, const std::string & name) : id_(id), 
            type_(type), volid_(volid), name_(name) {
        fds::PerfTracer::tracePointBegin(id_, type_, volid_, name_);
    }

    ~ScopedTracePoint_() {
        fds::PerfTracer::tracePointEnd(id_);
    }
private:
    const std::string & id_;
    const PerfEventType & type_;
    const fds_volid_t & volid_;
    const std::string & name_;
} ScopedTracePoint;

typedef struct ScopedTracePointCtx_ {
    ScopedTracePointCtx_(PerfContext & ctx) : ctx_(ctx) {
        fds::PerfTracer::tracePointBegin(ctx_);
    }

    ~ScopedTracePointCtx_() {
        fds::PerfTracer::tracePointEnd(ctx_);
    }
private:
    PerfContext & ctx_;
} ScopedTracePointCtx;

} /* namespace fds */

#endif
