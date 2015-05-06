/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <sys/types.h>
#include <sys/wait.h>

#include <cstdlib>
#include <cmath>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>  // NOLINT
#include <thread>

#include <fds_uuid.h>
#include <fdsp/svc_types_types.h>
#include <fds_process.h>
#include "fds_resource.h"
#include <platform/process.h>
#include "disk_plat_module.h"
#include <util/stringutils.h>

#include <fdsp/svc_types_types.h>

#include "platform/platform_manager.h"
#include "platform/disk_capabilities.h"

namespace fds
{
    namespace pm
    {
        // Setup the mapping for enums to string
        const std::map <int, std::string> m_idToAppNameMap =
        {
            { JAVA_AM,         JAVA_AM_CLASS_NAME },
            { BARE_AM,         BARE_AM_NAME       },
            { DATA_MANAGER,    DM_NAME            },
            { STORAGE_MANAGER, SM_NAME            }
        };

        PlatformManager::PlatformManager() : Module("pm"), m_appPidMap(), m_childMonitor(NULL)
        {

        }

        int PlatformManager::mod_init(SysParams const *const param)
        {
            conf = new FdsConfigAccessor(g_fdsprocess->get_conf_helper());
            rootDir = g_fdsprocess->proc_fdsroot()->dir_fdsroot();
            db = new kvstore::PlatformDB(rootDir, conf->get<std::string>("redis_host","localhost"), conf->get<int>("redis_port", 6379), 1);

            if (!db->isConnected())
            {
                LOGCRITICAL << "unable to talk to platformdb @ [" << conf->get<std::string>("redis_host","localhost") << ":" << conf->get<int>("redis_port", 6379) << "]";
            } else {
                db->getNodeInfo(nodeInfo);
                db->getNodeDiskCapability(diskCapability);
            }

            if (nodeInfo.uuid <= 0)
            {
                NodeUuid    uuid(fds_get_uuid64(get_uuid()));

                nodeInfo.uuid = uuid.uuid_get_val();
                nodeInfo.uuid = getNodeUUID(fpi::FDSP_PLATFORM);
                LOGNOTIFY << "generated a new uuid for this node : " << nodeInfo.uuid;
                db->setNodeInfo(nodeInfo);
            } else {
                LOGNOTIFY << "Using stored uuid for this node : " << nodeInfo.uuid;
            }

            determineDiskCapability();

            return 0;
        }

        void PlatformManager::mod_shutdown()
        {

        }

        pid_t PlatformManager::startProcess (int processID)
        {
            std::vector<std::string>    args;
            pid_t                       pid;
            std::string                 command;

            if (JAVA_AM == processID)
            {
                command = "java";
                args.push_back ("-classpath");
                args.push_back (JAVA_CLASSPATH_OPTIONS);

#ifdef DEBUG
                std::ostringstream remoteDebugger;
                remoteDebugger << JAVA_DEBUGGER_OPTIONS << conf->get<int>("platform_port") + 7777;
                args.push_back (remoteDebugger.str());
#endif // DEBUG

                args.push_back (JAVA_AM_CLASS_NAME);
            }
            else
            {
                command = m_idToAppNameMap.at(processID);
            }

            // Common command line options
            args.push_back("--foreground");
            args.push_back(util::strformat("--fds.pm.platform_uuid=%lld", getNodeUUID(fpi::FDSP_PLATFORM)));
            args.push_back(util::strformat("--fds.common.om_ip_list=%s", conf->get_abs<std::string>("fds.common.om_ip_list").c_str()));
            args.push_back(util::strformat("--fds.pm.platform_port=%d", conf->get<int>("platform_port")));

            pid = fds_spawn_service(command, rootDir, args, false);

            if (pid > 0)
            {
                LOGDEBUG << m_idToAppNameMap.at(processID) << " started by platformd as pid " << pid;
                // add to pid list
            }
            else
            {
                LOGERROR << "fds_spawn_service() for " << m_idToAppNameMap.at(processID) << " FAILED to start by platformd with errno=" << errno;
            }

            return pid;
        }

        bool PlatformManager::waitPid (pid_t const pid, long waitTimeoutNanoSeconds, bool monitoring)  // 1-%-9
        {
            int    status;
            pid_t  waitPidRC;

            timespec startTime;
            timespec timeNow;
            timespec endTime;

            clock_gettime (CLOCK_REALTIME, &startTime);

            endTime.tv_sec = startTime.tv_sec + (waitTimeoutNanoSeconds / NANO_SECONDS_IN_1_SECOND) + (startTime.tv_nsec + waitTimeoutNanoSeconds) / NANO_SECONDS_IN_1_SECOND;
            endTime.tv_nsec = (startTime.tv_nsec + waitTimeoutNanoSeconds) % NANO_SECONDS_IN_1_SECOND;

            do
            {
                waitPidRC = waitpid (pid, &status, WNOHANG);

                if (pid == waitPidRC)
                {
                    if (WIFEXITED (status))
                    {
                        LOGDEBUG << "pid " << pid << " exited normally with exit code " << WEXITSTATUS (status);
                    }
                    else if (WIFSIGNALED (status))
                    {
                        if (!monitoring)
                        {
                            LOGDEBUG << "pid " << pid << " exited via a signal (likely SIGKILL during a shutdown sequence)";
                        }
                        else
                        {
                            LOGDEBUG << "pid " << pid << " exited unexpectedly.";
                        }
                    }

                    return true;
                }

                usleep (50000);
                clock_gettime (CLOCK_REALTIME, &timeNow);
            }
            while (timeNow.tv_sec < endTime.tv_sec || (timeNow.tv_sec <= endTime.tv_sec && timeNow.tv_nsec < endTime.tv_nsec));

            return false;
        }

        void PlatformManager::stopProcess (int id, bool haveLock)
        {
            std::map <std::string, pid_t>::iterator mapIter = m_appPidMap.find (m_idToAppNameMap.at(id));

            if (m_appPidMap.end() == mapIter)
            {
                LOGERROR << "Unable to find pid for " << m_idToAppNameMap.at(id) << " in stopProcess()";
                return;
            }

            LOGDEBUG << "Preparing to stop " << m_idToAppNameMap.at(id) << " via kill(pid, SIGTERM)";

            pid_t pid = mapIter->second;

            // TODO(DJN): check for pid < 2 here and error

            int rc = kill (pid, SIGTERM);

            if (rc < 0)
            {
                LOGWARN << "Error sending signal (SIGTERM) to " << m_idToAppNameMap.at(id) << "(pid = " << pid << ") errno = " << rc << ", will follow up with a SIGKILL";
            }

            // Wait for the SIGTERM to shutdown the process, otherwise revert to using SIGKILL
            if (rc < 0 || false == waitPid (pid, 500000000))   // 1/2 seconds (in nanoseconds
            {
                rc = kill (pid, SIGKILL);

                if (rc < 0)
                {
                    LOGERROR << "Error sending signal (SIGKILL) to " << m_idToAppNameMap.at(id) << "(pid = " << pid << ") errno = " << rc << "";
                }

                waitPid (pid, 500000000);           // 1/2 seconds (in nanoseconds

                if (rc < 0)
                {
                    LOGERROR << "Error sending signal (SIGKILL) to " << m_idToAppNameMap.at(id) << "(pid = " << pid << ") errno = " << errno << "";
                }
            }

            m_appPidMap.erase (mapIter);
        }

        // plf_start_node_services
        // -----------------------
        //
        void PlatformManager::activateServices(const fpi::ActivateServicesMsgPtr &activateMsg)
        {
            pid_t    pid;

            auto     &info = activateMsg->info;

            std::lock_guard <decltype (m_pidMapMutex)> lock (m_pidMapMutex);

            if (info.has_sm_service)
            {
                pid = startProcess(STORAGE_MANAGER);

                if (pid < 2)
                {
                    LOGCRITICAL << "Failed to start:  " << m_idToAppNameMap.at(STORAGE_MANAGER);
                }
                else
                {
                    m_appPidMap[SM_NAME] = pid;
                    nodeInfo.fHasSm = true;
                }
            }

            if (info.has_dm_service)
            {
                pid = startProcess(DATA_MANAGER);

                if (pid < 2)
                {
                    LOGCRITICAL << "Failed to start:  " << m_idToAppNameMap.at(DATA_MANAGER);
                }
                else
                {
                    m_appPidMap[DM_NAME] = pid;
                    nodeInfo.fHasDm = true;
                }
            }

            if (info.has_am_service)
            {

                pid = startProcess(BARE_AM);

                if (pid < 2)
                {
                    LOGCRITICAL << "Failed to start:  " << m_idToAppNameMap.at(BARE_AM);
                }
                else
                {
                    m_appPidMap[BARE_AM_NAME] = pid;

                    pid = startProcess(JAVA_AM);

                    if (pid < 2)
                    {
                        LOGCRITICAL << "Failed to start:  " << m_idToAppNameMap.at(JAVA_AM);

                        stopProcess (BARE_AM, true);
                    }
                    else
                    {
                        m_appPidMap[JAVA_AM_CLASS_NAME] = pid;
                        nodeInfo.fHasAm = true;
                    }
                }
            }

            db->setNodeInfo(nodeInfo);
        }

        void PlatformManager::deactivateServices(const fpi::DeactivateServicesMsgPtr &deactivateMsg)
        {
            std::lock_guard <decltype (m_pidMapMutex)> lock (m_pidMapMutex);

            if (deactivateMsg->deactivate_am_svc && nodeInfo.fHasAm)
            {
                stopProcess(JAVA_AM);
                stopProcess(BARE_AM);
            }

            if (deactivateMsg->deactivate_dm_svc && nodeInfo.fHasDm)
            {
                stopProcess(DATA_MANAGER);
            }

            if (deactivateMsg->deactivate_sm_svc && nodeInfo.fHasSm)
            {
                stopProcess(STORAGE_MANAGER);
            }
        }

        void PlatformManager::updateServiceInfoProperties(std::map<std::string, std::string> *data)
        {
            determineDiskCapability();
            util::Properties props = util::Properties(data);
            props.set("fds_root", rootDir);
            props.setInt("uuid", nodeInfo.uuid);
            props.setInt("node_iops_max", diskCapability.node_iops_max);
            props.setInt("node_iops_min", diskCapability.node_iops_min);
            props.setDouble("disk_capacity", diskCapability.disk_capacity);
            props.setDouble("ssd_capacity", diskCapability.ssd_capacity);
            props.setInt("disk_type", diskCapability.disk_type);
        }

        // TODO: this needs to populate real data from the disk module labels etc.
        // it may want to load the value from the database and validate it against
        // DiskPlatModule data, or just load from the DiskPlatModule and be done
        // with it.  Depends somewhat on how expensive it is to traverse the DiskPlatModule
        // and calculate all the data.
        void PlatformManager::determineDiskCapability()
        {
            auto ssd_iops_max = conf->get<uint32_t>("capabilities.disk.ssd.iops_max");
            auto ssd_iops_min = conf->get<uint32_t>("capabilities.disk.ssd.iops_min");
            auto hdd_iops_max = conf->get<uint32_t>("capabilities.disk.hdd.iops_max");
            auto hdd_iops_min = conf->get<uint32_t>("capabilities.disk.hdd.iops_min");
            auto space_reserve = conf->get<float>("capabilities.disk.reserved_space");

            DiskPlatModule* dpm = DiskPlatModule::dsk_plat_singleton();
            auto disk_counts = dpm->disk_counts();

            if (0 == (disk_counts.first + disk_counts.second)) {
                // We don't have real disks
                diskCapability.disk_capacity = 0x7ffff;
                diskCapability.ssd_capacity = 0x10000;
            } else {

                // Calculate aggregate iops with both HDD and SDD
                diskCapability.node_iops_max  = (hdd_iops_max * disk_counts.first);
                diskCapability.node_iops_max += (ssd_iops_max * disk_counts.second);

                diskCapability.node_iops_min  = (hdd_iops_min * disk_counts.first);
                diskCapability.node_iops_min += (ssd_iops_min * disk_counts.second);

                // We assume all disks are connected identically atm
                diskCapability.disk_type = dpm->disk_type() ? FDS_DISK_SAS : FDS_DISK_SATA;

                auto disk_capacities = dpm->disk_capacities();
                diskCapability.disk_capacity = (1.0 - space_reserve) * disk_capacities.first;
                diskCapability.ssd_capacity = (1.0 - space_reserve) * disk_capacities.second;
            }

            if (conf->get<bool>("testing.manual_nodecap",false))
            {
                diskCapability.node_iops_max    = conf->get<int>("testing.node_iops_max", 100000);
                diskCapability.node_iops_min    = conf->get<int>("testing.node_iops_min", 6000);
            }
            LOGDEBUG << "Set node iops max to: " << diskCapability.node_iops_max;
            LOGDEBUG << "Set node iops min to: " << diskCapability.node_iops_min;

            db->setNodeDiskCapability(diskCapability);
        }

        fds_int64_t PlatformManager::getNodeUUID(fpi::FDSP_MgrIdType svcType)
        {
            ResourceUUID    uuid;
            uuid.uuid_set_type(nodeInfo.uuid, svcType);

            return uuid.uuid_get_val();
        }

        void PlatformManager::childProcessMonitor()
        {
            LOGDEBUG << "Starting thread for PlatformManager::childProcessMonitor()";

            uint32_t count = 0;
            uint32_t lastCount = 0;

            while (true)
            {
                {   // Create a context for the lock_guard
                    std::lock_guard <decltype (m_pidMapMutex)> lock (m_pidMapMutex);
#ifdef DEBUG
                    count = m_appPidMap.size();
                    if (count != lastCount)
                    {
                        LOGDEBUG << "Now monitoring " << count << " children (was " << lastCount << ")";
                        lastCount = count;
                    }
#endif
                    std::map <std::string, pid_t>::iterator mapIter = m_appPidMap.begin();

                    while (m_appPidMap.end() != mapIter)
                    {
                        if (waitPid (mapIter->second, 1000, true))
                        {
                            m_appPidMap.erase (mapIter++);
                            continue;
                        }
                        else
                        {
                            mapIter++;
                        }
                    }
                }  // lock_guard context

                usleep (500000);
            }
        }

        void PlatformManager::run()
        {
            m_childMonitor = new std::thread (&PlatformManager::childProcessMonitor, this);

            while (1)
            {
                sleep(999);   /* we'll do hotplug uevent thread in here */
            }
        }
    }  // namespace pm
}  // namespace fds
