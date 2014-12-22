/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_PLATFORM_PLATFORM_PROCESS_H_
#define SOURCE_INCLUDE_PLATFORM_PLATFORM_PROCESS_H_

#include <string>

#include <kvstore/platformdb.h>
#include "fds_process.h"
#include "platform/plat_node_data.h"

namespace fds
{
    class PlatformDB;

    /**
     * FDS Platform daemon process.
     */
    class PlatformProcess : public FdsProcess
    {
        public:
            virtual ~PlatformProcess();
            PlatformProcess();
            PlatformProcess(int argc, char *argv[], const std::string &cfg_path,
                            const std::string &log_file, Platform *platform, Module **vec);
            void init(int argc, char **argv, const std::string  &cfg, const std::string  &log,
                      Platform *platform, Module **vec);

            virtual void proc_pre_startup() override;

            /* Override from CommonModuleProviderIf */
            virtual Platform* get_plf_manager() override
            {
                return plf_mgr;
            }

            /**
             * Return platform manager from the global singleton.
             */
            static inline Platform *plf_manager()
            {
                return static_cast<PlatformProcess *>(g_fdsprocess)->plf_mgr;
            }

        protected:
            Platform              *plf_mgr;
            kvstore::PlatformDB   *plf_db;

            fds_bool_t             plf_test_mode;
            fds_bool_t             plf_stand_alone;
            std::string            plf_db_key;
            plat_node_data_t       plf_node_data;

            virtual void plf_load_node_data();
            virtual void plf_apply_node_data();
    };
}  // namespace fds

#endif  // SOURCE_INCLUDE_PLATFORM_PLATFORM_PROCESS_H_
