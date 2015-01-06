/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_PROBE_PROCESS_H_
#define SOURCE_INCLUDE_PLATFORM_PROBE_PROCESS_H_

#include <string>

namespace fds
{
    class ProbeProcess : public FdsProbeProcess
    {
        public:
            virtual ~ProbeProcess();
            ProbeProcess(int argc, char *argv[], const std::string &log, ProbeMod *probe,
                         Module **vec, const std::string &cfg = "fds.plat.");
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_PROBE_PROCESS_H_
