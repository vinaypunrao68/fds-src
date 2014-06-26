#ifndef __RDTSC_H__
#define __RDTSC_H__
#include <stdint.h> /* for uint64_t */
#include <time.h>  /* for struct timespec */
#include <util/timeutils.h>

const int NANO_SECONDS_IN_SEC = 1000000000;
/* returns a static buffer of struct timespec with the time difference of ts1 and ts2
   ts1 is assumed to be greater than ts2 */
struct timespec *TimeSpecDiff(struct timespec *ts1, struct timespec *ts2)
{
    static struct timespec ts;
    ts.tv_sec = ts1->tv_sec - ts2->tv_sec;
    ts.tv_nsec = ts1->tv_nsec - ts2->tv_nsec;
    if (ts.tv_nsec < 0) {
        ts.tv_sec--;
        ts.tv_nsec += NANO_SECONDS_IN_SEC;
    }
    return &ts;
}

double g_TicksPerNanoSec;
static void CalibrateTicks()
{
    struct timespec begints, endts;
    uint64_t begin = 0, end = 0;
    clock_gettime(CLOCK_MONOTONIC, &begints);
    begin = fds::util::getClockTicks();
    uint64_t i;
    for (i = 0 ; i < 1000000 ; i++){} /* must be CPU intensive */
    end = fds::util::getClockTicks();
    clock_gettime(CLOCK_MONOTONIC, &endts);
    struct timespec *tmpts = TimeSpecDiff(&endts, &begints);
    uint64_t nsecElapsed = tmpts->tv_sec * 1000000000LL + tmpts->tv_nsec;
    g_TicksPerNanoSec = (double)(end - begin)/(double)nsecElapsed;
}

/* Call once before using RDTSC, has side effect of binding process to CPU1 */
void InitRdtsc()
{
    CalibrateTicks();
}

void GetTimeSpec(struct timespec *ts, uint64_t nsecs)
{
    ts->tv_sec = nsecs / NANO_SECONDS_IN_SEC;
    ts->tv_nsec = nsecs % NANO_SECONDS_IN_SEC;
}

/* ts will be filled with time converted from TSC reading */
void GetRdtscTime(struct timespec *ts)
{
    GetTimeSpec(ts, fds::util::getClockTicks() / g_TicksPerNanoSec);
}



#endif
