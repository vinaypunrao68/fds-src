/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_FDS_SERVICE_H_
#define SOURCE_INCLUDE_PLATFORM_FDS_SERVICE_H_

#include <string>

#include "platform/platform_process.h"

namespace fds
{
    /**
     * FDS Service Process, used for testing and stand-alone app.
     */
    class FdsService : public PlatformProcess
    {
        public:
            virtual ~FdsService();

            FdsService() : PlatformProcess(), svc_modules(NULL)
            {
            }

            FdsService(int argc, char *argv[], const std::string &log, Module **vec);

            virtual void proc_pre_startup() override;
            virtual void proc_pre_service() override;

        protected:
            Module  **svc_modules;
            virtual void plf_load_node_data();
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_FDS_SERVICE_H_
