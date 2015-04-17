#include <unistd.h>

#include <vector>
#include <chrono>
#include <thread>
#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

#include "fds_process.h"
#include "PerfTrace.h"
//#include "PerfTypes.h"

// XXX: uncomment to run single threaded
// #define RUN_IN_SINGLE_THREAD

// XXX: modify this to run with fixed sleep
const int TASK_SLEEP_TIME = -1;

/**
 * Unit test for PerfTracer
 *
 * The tests starts NUM_SIM_JOBS number of threads simultaneously in a loop
 * up to MAX_PERF_JOBS number of jobs. All those threads perform different
 * operations on PerfTracer (increment NumericCounter, start trace with local
 * context, start trace with dynamic context etc). For LatencyCounters, task()
 * begins trace, sleeps for certain time and then ends trace. In the end,
 * all collected counters are printed.
 */

using namespace fds;

const unsigned NUM_SIM_JOBS = 20;  // 20
const unsigned MAX_PERF_JOBS = 1000;  // 1000

const char * PREFIX[] = {"perf-ut-", "trace-ut", "test-ut-"};

typedef struct JobDetails_ {
    PerfEventType type;
    fds_volid_t volid;
    unsigned delay;
    std::string id;
    PerfContext ctx;
} JobDetails;

class PerfTraceUTProc : public FdsProcess {
public:
    PerfTraceUTProc(int argc, char * argv[], const std::string & config,
            const std::string & basePath, Module * vec[])
            : FdsProcess(argc, argv, config, basePath, vec),
              jobs_(MAX_PERF_JOBS),
              threads_(MAX_PERF_JOBS) {}
    virtual ~PerfTraceUTProc() {}

    virtual int run() override;

    virtual void task(int id);

private:
    std::vector<JobDetails> jobs_;
    std::vector<std::thread> threads_;
};

int PerfTraceUTProc::run() {
    for (unsigned i = 0; i < MAX_PERF_JOBS; ++i) {
        jobs_[i].type = static_cast<PerfEventType>( rand() % (fds_enum::get_size<PerfEventType>() - 2) + 2);
        jobs_[i].volid = static_cast<fds_volid_t>(rand());
        jobs_[i].delay = TASK_SLEEP_TIME < 0 ? (rand() % 31) + 1 : TASK_SLEEP_TIME;
        jobs_[i].id = PREFIX[rand() % 3] + boost::lexical_cast<std::string>(i);
        jobs_[i].ctx.type = jobs_[i].type;
        jobs_[i].ctx.name = jobs_[i].id;
        jobs_[i].ctx.reset_volid(jobs_[i].volid);

#ifdef RUN_IN_SINGLE_THREAD
        task(i);
#else
        threads_[i] = std::thread(&PerfTraceUTProc::task, this, i);

        if (!(i % NUM_SIM_JOBS)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 31));
        }
#endif
    }

#ifndef RUN_IN_SINGLE_THREAD
    // Wait for all threads
    for (unsigned x = 0; x < MAX_PERF_JOBS; ++x) {
        threads_[x].join();
    }
#endif

    // Print all counter values
    std::cout << "------------------------------\n";
    g_cntrs_mgr->export_to_ostream(std::cout);
    syncfs(STDOUT_FILENO);

    return 0;
}

void PerfTraceUTProc::task(int id) {
    // begin perf trace
    unsigned x = fds_enum::get_index<PerfEventType>(jobs_[id].type) % 2;
    unsigned y = fds_enum::get_index<PerfEventType>(jobs_[id].type) % 4;

    std::cout << "begin - task=" << jobs_[id].id << " type=" << jobs_[id].type <<
        " volid=" << jobs_[id].volid <<
        " delay=" << jobs_[id].delay << std::endl;
    if (!x) {
        // all even numbered events use LatencyCounter
        if (!y) {
            // use local PerfContext
            PerfTracer::tracePointBegin(jobs_[id].ctx);
        } else {
            // use dynamic PerfContext
            PerfTracer::tracePointBegin(jobs_[id].id, jobs_[id].type, jobs_[id].volid, jobs_[id].id);
        }
    } else {
        // all odd numbered event use Numeric counter
        PerfTracer::incr(jobs_[id].type, jobs_[id].volid, jobs_[id].id);
    }

    // task is executed here
    std::this_thread::sleep_for(std::chrono::milliseconds(jobs_[id].delay));

    // end perf trace
    boost::shared_ptr<PerfContext> pc;
    if (!x) {
        if (!y) {
            PerfTracer::tracePointEnd(jobs_[id].ctx);
        } else {
            pc = PerfTracer::tracePointEnd(jobs_[id].id);
        }

        std::cout << "end - task=" << jobs_[id].id << " type=" << jobs_[id].type <<
            " volid=" << jobs_[id].volid <<
            " delay=" << jobs_[id].delay << " latency=";
        LatencyCounter * plc = dynamic_cast<LatencyCounter *>(pc ? pc->data.get() :
                jobs_[id].ctx.data.get());
        std::cout << plc->latency() << std::endl;
    }
}

int main(int argc, char * argv[]) {
    PerfTraceUTProc p(argc, argv, "perf_ut.conf", "fds.perf_ut.", NULL);
    std::cout << "unit test " << __FILE__ << " started." << std::endl;
    p.main();
    std::cout << "unit test " << __FILE__ << " finished." << std::endl;
    return 0;
}

