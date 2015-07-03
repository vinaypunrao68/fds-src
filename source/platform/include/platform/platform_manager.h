/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_PLATFORM_MANAGER_H_
#define SOURCE_PLATFORM_INCLUDE_PLATFORM_MANAGER_H_

#include <mutex>

#include <fds_config.hpp>
#include <fds_module.h>
#include <kvstore/platformdb.h>
#include <fdsp/pm_data_types.h>
#include <fds_typedefs.h>
#include <disk_plat_module.h>

#include "fdsp/node_svc_api_types.h"
#include "fdsp/pm_service_types.h"

namespace fds
{
    namespace pm
    {
        enum
        {
            JAVA_AM,
            BARE_AM,
            DATA_MANAGER,
            STORAGE_MANAGER
        };

        const std::string JAVA_AM_CLASS_NAME = "com.formationds.am.Main";
        const std::string BARE_AM_NAME       = "bare_am";
        const std::string DM_NAME            = "DataMgr";
        const std::string SM_NAME            = "StorMgr";
        const std::string JAVA_PROCESS_NAME  = "java";

        const std::string JAVA_CLASSPATH_OPTIONS      = "/lib/java/*";  // No spaces in this value. ${FDS_ROOT} will be prepended.

        constexpr uint64_t NANO_SECONDS_IN_1_SECOND   = 1000000000;
        constexpr uint64_t PROCESS_STOP_WAIT_PID_SLEEP_TIMER_NANOSECONDS = 500000000;         // 500,000,000 = 1/2 sconds
        constexpr useconds_t WAIT_PID_SLEEP_TIMER_MICROSECONDS = 50000;
        constexpr useconds_t PROCESS_MONITOR_SLEEP_TIMER_MICROSECONDS = 500000;               // 500,000 = Every 1/2 second

        // PROC_CHECK_BITMASK is a bit used to indicate to the process monitor that any children processes with this bit set have been
        // inherited by initd  and they must be monitored via the /proc file system rather than as children since they are orphans.
        constexpr pid_t PROC_CHECK_BITMASK = (1 << (sizeof (pid_t) * 8 - 2));

        constexpr pid_t EMPTY_PID = -1;

#ifdef DEBUG
        const std::string JAVA_DEBUGGER_OPTIONS       = "-agentlib:jdwp=transport=dt_socket,server=y,suspend=n,address=";             // No spaces in this value
#endif // DEBUG

        class PlatformManager : public Module
        {
            public:
                PlatformManager();

                /* Overrides from Module */
                virtual int  mod_init (SysParams const *const param) override;
                virtual void mod_startup() override
                {
                }

                virtual void mod_shutdown() override
                {
                }

                void run();

                void activateServices (const fpi::ActivateServicesMsgPtr &activateMsg);
                void deactivateServices (const fpi::DeactivateServicesMsgPtr &deactivateMsg);

                const fpi::NodeInfo& getNodeInfo()
                {
                    return m_nodeInfo;
                }

                void addService (fpi::NotifyAddServiceMsgPtr const &addNodeMsg);
                void removeService (fpi::NotifyRemoveServiceMsgPtr const &removeServiceMsg);

                void startService (fpi::NotifyStartServiceMsgPtr const &startServiceMsg);
                void stopService (fpi::NotifyStopServiceMsgPtr const &stopServiceMsg);

                /**
                 * Update the service info properties with disk information,
                 * the node uuid, and fds_root.
                 */
                void updateServiceInfoProperties (std::map<std::string, std::string> *data);

            protected:
                fds_uint64_t getNodeUUID (fpi::FDSP_MgrIdType svcType);

                void determineDiskCapability();

                bool waitPid (pid_t pid, uint64_t waitTimeoutNanoSeconds, bool monitoring = false);
                void startProcess (int id);
                void stopProcess (int id);

            private:
                FdsConfigAccessor                  *fdsConfig;
                fpi::FDSP_AnnounceDiskCapability    diskCapability;

                kvstore::PlatformDB                *m_db;
                fpi::NodeInfo                       m_nodeInfo;
                std::string                         rootDir;

                std::mutex                          m_pidMapMutex;
                std::map <std::string, pid_t>       m_appPidMap;

                std::mutex                          m_startQueueMutex;
                std::condition_variable             m_startQueueCondition;
                std::list <int>                     m_startQueue;

                bool                                m_autoRestartFailedProcesses;
                bool                                m_startupAuditComplete;        // Tracks if the run function has completed it's startup audit.
                                                                                   // which prevents service activate function from occurring.

                std::string                         m_nodeRedisKeyId;              // A unique id across that the nodes in a cluster, this is differentthen the
                                                                                   // node UUID used in other places.  This persists across cleans, reboots, etc.

                void loadRedisKeyId();
                void childProcessMonitor();
                void startQueueMonitor();
                void notifyOmAProcessDied (std::string const &procName, int const appIndex, pid_t const procPid);
                std::string getProcName (int const index);
                void updateNodeInfoDbPid (int processType, pid_t pid);
                void updateNodeInfoDbState (int processType, fpi::pmServiceStateTypeId newState);
                void checkPidsDuringRestart();
                bool procCheck (std::string procName, pid_t pid);
        };
    }  // namespace pm
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_PLATFORM_MANAGER_H_
