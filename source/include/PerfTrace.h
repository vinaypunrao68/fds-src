#ifndef SOURCE_PERF_TRACE_H_
#define SOURCE_PERF_TRACE_H_

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>

#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>

#include <utility>
#include <vector>
#include <unordered_map>
#include <bitset>

#include "fds_types.h"
#include "fds_counters.h"
#include "fds_config.hpp"
#include "util/Log.h"
#include "concurrency/Mutex.h"

namespace fds { 

// XXX: update eventTypeToStr[] for each event added
extern const char * eventTypeToStr[];
typedef enum {
    TRACE, // generic event

    QOS,
    MURMUR3_HASH,
    DLT_LKUP,
    DMT_LKUP,
    PUT_OBJ,
    GET_OBJ,
    
    PUT_OBJ_DEDUPE_CHK,
    PERSIST_DISK_WRITE,
    PUT_OBJ_LOC_INDX_UPDATE,

    GET_OBJ_CACHE_LKUP,
    GET_OBJ_LKUP_LOC_INDX,
    GET_OBJ_PL_READ_DISK,

    // XXX: add new entries before this entry
    MAX_EVENT_TYPE // this should be the last entry in the enum
} PerfEventType;

typedef struct PerfContext_ {
    PerfContext_() :
            type(TRACE),
            name(""),
            enabled(true),
            start_cycle(0),
            end_cycle(0),
            data(0) {}

    PerfContext_(PerfEventType type_, std::string name_ = "") :
            type(type_),
            name(name_),
            enabled(true),
            start_cycle(0),
            end_cycle(0),
            data(0) {
        fds_assert(type < MAX_EVENT_TYPE);
    }

    inline void generateLatency();

    PerfEventType type;
    std::string name;
    bool enabled;
    fds_uint64_t start_cycle;
    fds_uint64_t end_cycle;
    boost::shared_ptr<FdsBaseCounter> data;
} PerfContext;

// Per module (SM/DM/SH) perf tracer 
class PerfTracer {
public:

    // Increment NumericCounter by 1
    static inline void incr(const PerfEventType & type, std::string name = "");

    // Increment counter value by val and (for LatencyCounter, count by cnt)
    static void incr(const PerfEventType & type, fds_uint64_t val = 0,
            fds_uint64_t cnt = 0, std::string name = "");

    // update begin timestamp for identified event
    static void tracePointBegin(const std::string & id, const PerfEventType & type,
            std::string name = "");
    static inline void tracePointBegin(PerfContext & ctx);

    // update end timestamp for identified context and updates latency
    static boost::shared_ptr<PerfContext> tracePointEnd(const std::string & id);
    static void tracePointEnd(PerfContext & ctx);

    // enable/ disable performance data collection
    static inline void setEnabled(bool val = true);
    static inline bool isEnabled();

    // reload the configuration
    static inline void refresh();

    PerfTracer();
    ~PerfTracer();

private:

    typedef std::unordered_map<std::string, PerfContext *> PerfContextMap;

    // map maintaining LatencyCounters for tracePointBegin()/ tracePointEnd()
    PerfContextMap latencyMap_;
    fds_mutex latency_map_mutex_; //only for latencyMap_

    // all the counters
    boost::shared_ptr<FdsCounters> exportedCounters;

    std::vector<PerfContext> aggregateCounters_;

    std::vector<PerfContextMap> nameCounters_;
    fds_mutex ptrace_mutex_; // only for nameCounters_

    /*
     * configuration
     */
    // enable/ disable performace data collection
    bool enable_;

    // events filter for event type
    std::bitset<MAX_EVENT_TYPE> eventsFilter_;
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
    PerfTracer(const PerfTracer & rhs);
    PerfTracer & operator =(const PerfTracer & rhs);

    void reconfig();

    void updateCounter(PerfContext & ctx, const fds_uint64_t & val, const fds_uint64_t cnt);

    // update & insert (if counter is not present)
    void upsert(const PerfEventType & type, fds_uint64_t val, fds_uint64_t cnt,
            const std::string & name);

    static inline PerfTracer & instance();
};

} /* namespace fds */

#endif
