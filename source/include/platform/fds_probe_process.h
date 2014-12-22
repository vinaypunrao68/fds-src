/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_FDS_PROBE_PROCESS_H_
#define SOURCE_INCLUDE_PLATFORM_FDS_PROBE_PROCESS_H_

#include <string>

namespace fds
{
    /*
     * Put the probe module embeded inside a FDS process (e.g. SM/DM/AM/OM).
     */
    class FdsProbeProcess : public FdsService
    {
        public:
            virtual ~FdsProbeProcess();

            FdsProbeProcess() : FdsService(), svc_probe(NULL)
            {
            }

            FdsProbeProcess(int argc, char *argv[], const std::string &cfg, const std::string &log,
                            ProbeMod *probe, Platform *plat, Module **mod);

            virtual void proc_pre_startup() override;
            virtual void proc_pre_service() override;

        protected:
            ProbeMod   *svc_probe;
    };

}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_FDS_PROBE_PROCESS_H_
