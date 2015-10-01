/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <util/timeutils.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <cstring>

#include <util/stringutils.h>

namespace fds {
namespace util {
fds_uint64_t CYCLES_PER_SECOND = CLOCKS_PER_SEC;
TimeStamp getTimeStampNanos() {
    timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (TimeStamp)t.tv_sec * 1000000000ull + (TimeStamp)t.tv_nsec;
}

TimeStamp getTimeStampMicros() {
    timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (TimeStamp)t.tv_sec * 1000000ull + (TimeStamp)t.tv_nsec/1000;
}

TimeStamp getTimeStampMillis() {
    timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (TimeStamp)t.tv_sec * 1000ull + (TimeStamp)t.tv_nsec/1000000;
}

TimeStamp getTimeStampSeconds() {
    time_t  tt;
    time(&tt);
    return tt;
}

std::string getLocalTimeString(TimeStamp t) {
    time_t tt = (time_t)t;
    char value[26];
    asctime_r(localtime(&tt),value); // NOLINT
    std::strchr(value,'\n')[0] = 0;
    return value;
}

std::string getGMTimeString(TimeStamp t) {
    time_t tt = (time_t)t;
    char value[26];
    asctime_r(gmtime(&tt), value); // NOLINT
    std::strchr(value,'\n')[0] = 0;
    return value;
}


fds_uint64_t rdtsc() {
    unsigned int hi, lo;
    __asm__ volatile("rdtsc" : "=a" (lo), "=d" (hi));
    return ((fds_uint64_t)hi << 32) | lo;
}

fds_uint64_t rdtsc_barrier() {
#ifdef __i386__
    fds_uint64_t tsc asm("eax");
    asm volatile(".byte 15, 49" : : : "eax", "edx");
    return tsc;
#else
    fds_uint64_t hi, lo;

    asm volatile("cpuid; rdtscp" : "=a"(lo), "=d"(hi));
    return ((hi << 32) | lo);
#endif
}

fds_uint64_t getCpuSpeedHz() {
    const int buf_len = 2096;
    double mhz;
    int    len, fd;
    char   buf[buf_len], *p;

    fd = open("/proc/cpuinfo", O_RDONLY);
    if (fd < 0) {
        goto def;
    }
    len = read(fd, buf, buf_len);
    close(fd);
    if (len <= 0) {
        goto def;
    }
    for (p = buf; *p != '\0'; p++) {
        if (p[0] == 'c' && p[1] == 'p' && p[2] == 'u' && p[3] == ' ' &&
            p[4] == 'M' && p[5] == 'H' && p[6] == 'z') {
            for (p += 6; *p != '\0' && *p != ':'; p++);
            if (*p != ':') {
                goto def;
            }
            mhz = atof(p + 1);
            if (mhz < 1) {
                goto def;
            }
            return fds_uint64_t (mhz * 1000000);
            break;
        }
    }
    return 0;

def:
    return CLOCKS_PER_SEC;
}

// this will automatically initalize the variable on load...
struct __INIT_CYCLES__ {
    __INIT_CYCLES__() {
        CYCLES_PER_SECOND = getCpuSpeedHz();
    }
} ___init__cycles__;


fds_uint64_t getClockTicks() {
    return rdtsc();
}

TimeStamp getNanosFromTicks(fds_uint64_t ticks) {
    return ticks / (CYCLES_PER_SECOND / (1000000 * 1000));
}

/**
 * Return seconds from human readable time
 * input string as: case-insensitive
 * "12345",""12345s", "12345sec" = 12345
 * "1h", "1 hour" = 3600
 * "60m", "60mins" = 3600
 * "2d", "2 days" = 2*86400
 * "1w", "1 week" = 7 * 86400
 *  MM:SS = MM*60 + SS
 *  HH:MM:SS = HH*3600 + MM*60 + SS
 */
TimeStamp getSecondsFromHumanTime(const std::string& strTime) {
    // check if : is present in the string , then it is HH:MM:SS
    TimeStamp seconds = 0;
    if (std::string::npos != strTime.find(':')) {
        std::vector<std::string> tokens;
        tokenize(strTime, tokens, ':');

        if (tokens.size() > 3 || tokens.size() < 2) {
            throw std::invalid_argument(strformat("time in wrong hh:mm:ss format - [%s]",strTime.c_str()));
        }

        uint mults[] = {1, 60, 3600};
        uint pos;
        for (uint i = 0 ; i < tokens.size(); i++) {
            pos = tokens.size() - i - 1 ;
            seconds += std::atoi(tokens[pos].c_str()) * mults[i];
        }
        return seconds;
    }

    int mult = 1;
    char *p, *end;
    seconds =  std::strtol(strTime.c_str(), &end, 10);
    p = end;
    for ( ; *p; ++p) *p = tolower(*p);
    if (end != strTime.c_str()) {
        if (0 == strncmp(end, "s", 1)) mult = 1;
        else if (0 == strncmp(end, "m", 1)) mult = 60;
        else if (0 == strncmp(end, "h", 1)) mult = 3600;
        else if (0 == strncmp(end, "d", 1)) mult = 86400;
        else if (0 == strncmp(end, "w", 1)) mult = 86400 * 7;
    }
    return seconds * mult;
}

// ==========================================================================================

TimeStamp StopWatch::start() {
    startTime = getTimeStampNanos();
    return startTime;
}

void StopWatch::reset() {
    startTime = getTimeStampNanos();
}

TimeStamp StopWatch::getElapsedNanos() {
    return getTimeStampNanos() - startTime;
}

void TimeTracker::start() {
    stopWatch.start();
}

TimeStamp TimeTracker::mark(const std::string& name) {
    TimeStamp elapsed = stopWatch.getElapsedNanos();
    vecTimes.push_back(make_pair(name, elapsed));
    return elapsed;
}

void  TimeTracker::done() {
    mark("--end--");
}

TimeStamp TimeTracker::getNanosAt(const std::string& name) {
    for (const auto& item : vecTimes) {
        if (name == item.first) return item.second;
    }

    return 0;
}

TimeStamp TimeTracker::totalNanos() {
    if (vecTimes.empty()) return 0;
    return vecTimes[vecTimes.size()-1].second;
}

std::ostream& operator<<(std::ostream& oss, const fds::util::TimeTracker& tracker) {
    oss << "Times:\n";
    auto iter = tracker.vecTimes.begin();
    fds::util::TimeStamp previous {0};

    if (iter != tracker.vecTimes.end()) {
        oss << iter->first << ":" << iter->second << "\n";
        previous = iter->second;
        ++iter;
    }

    while (iter != tracker.vecTimes.end()) {
        oss << iter->first << ":" << iter->second - previous << ":" << iter->second << "\n";
        previous = iter->second;
        ++iter;
    }
    return oss;
}

}  // namespace util
}  // namespace fds
