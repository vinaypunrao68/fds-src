/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <sys/types.h>
#include <sys/wait.h>

#include <stdlib.h>
#include <string>
#include <vector>
#include <sstream>
#include <iostream>  // NOLINT

#include <fds_uuid.h>
#include <fdsp/svc_types_types.h>
#include <fds_process.h>
#include <platform/process.h>
#include <util/stringutils.h>

#include <fdsp/svc_types_types.h>

#include "platform/platform_manager.h"

namespace fds
{
    namespace pm
    {
        PlatformManager::PlatformManager() : Module("pm"), m_idToAppNameMap(), m_appPidMap()
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

            // Setup the mapping for enums to string
            m_idToAppNameMap[JAVA_AM]         = JAVA_AM_CLASS_NAME;
            m_idToAppNameMap[BARE_AM]         = BARE_AM_NAME;
            m_idToAppNameMap[DATA_MANAGER]    = DM_NAME;
            m_idToAppNameMap[STORAGE_MANAGER] = SM_NAME;

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
                args.push_back (JAVA_AM_CLASS_NAME);
            }
            else
            {
                command = m_idToAppNameMap[processID];
            }

            // Common command line options
            args.push_back("--foreground");
            args.push_back(util::strformat("--fds.pm.platform_uuid=%lld", getNodeUUID(fpi::FDSP_PLATFORM)));
            args.push_back(util::strformat("--fds.common.om_ip_list=%s", conf->get_abs<std::string>("fds.common.om_ip_list").c_str()));
            args.push_back(util::strformat("--fds.pm.platform_port=%d", conf->get<int>("platform_port")));

            pid = fds_spawn_service(command, rootDir, args, false);

            if (pid > 0)
            {
                LOGDEBUG << m_idToAppNameMap[processID] << " started by platformd as pid " << pid;
                // add to pid list
            }
            else
            {
                LOGERROR << "fds_spawn_service() for " << m_idToAppNameMap[processID] << " FAILED to start by platformd with errno=" << errno;
            }

            return pid;
        }

        bool PlatformManager::waitPid (pid_t const pid, int waitTimeoutNanoSeconds)  // 1-%-9
        {
            int    status;
            pid_t  waitPidRC;

            time_t timeNow = time(NULL);
            int timeEnd = timeNow + 2;

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
                        LOGDEBUG << "pid " << pid << " exited via a signal (likely SIGKILL during a shutdown sequence)";
                    }

                    return true;
                }

                usleep (100000);
                timeNow = time(NULL);
            }
            while (timeNow < timeEnd);

            return false;
        }

        void PlatformManager::stopProcess (int id, bool haveLock)
        {
            LOGDEBUG << "Attempting to Stop " << m_idToAppNameMap[id] << " via kill(pid, SIGTERM)";

            std::map <std::string, pid_t>::iterator mapIter = m_appPidMap.find (m_idToAppNameMap[id]);

            if (m_appPidMap.end() == mapIter)
            {
                LOGERROR << "Unable to find pid for " << m_idToAppNameMap[id] << " in stopProcess()";
                return;
            }

            // TODO(DJN): check for pid < 2 here and error

            pid_t pid = mapIter->second;

            int rc = kill (pid, SIGTERM);

            if (rc < 0)
            {
                LOGWARN << "Error sending signal (SIGTERM) to " << m_idToAppNameMap[id] << "(pid = " << pid << ") errno = " << rc << ", will follow up with a SIGKILL";
            }

            // Wait for the SIGTERM to shutdown the process, otherwise revert to using SIGKILL
            if (false == waitPid (pid, 9))
            {
                rc = kill (pid, SIGKILL);

                if (rc < 0)
                {
                    LOGERROR << "Error sending signal (SIGKILL) to " << m_idToAppNameMap[id] << "(pid = " << pid << ") errno = " << rc << "";
                }

                waitPid (pid, 9);
            }

            if (!haveLock)
            {
                std::lock_guard <decltype (m_pidMapMutex)> lock (m_pidMapMutex);
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
                    LOGCRITICAL << "Failed to start:  " << m_idToAppNameMap[STORAGE_MANAGER];
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
                    LOGCRITICAL << "Failed to start:  " << m_idToAppNameMap[DATA_MANAGER];
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
                    LOGCRITICAL << "Failed to start:  " << m_idToAppNameMap[BARE_AM];
                }
                else
                {
                    m_appPidMap[BARE_AM_NAME] = pid;

                    pid = startProcess(JAVA_AM);

                    if (pid < 2)
                    {
                        LOGCRITICAL << "Failed to start:  " << m_idToAppNameMap[JAVA_AM];

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

        void PlatformManager::loadProperties()
        {
            props.set("fds_root", rootDir);
            props.setInt("uuid", nodeInfo.uuid);
            props.setInt("disk_iops_max", diskCapability.disk_iops_max);
            props.setInt("disk_iops_min", diskCapability.disk_iops_min);
            props.setDouble("disk_capacity", diskCapability.disk_capacity);
            props.setInt("disk_latency_max", diskCapability.disk_latency_max);
            props.setInt("disk_latency_min", diskCapability.disk_latency_min);
            props.setInt("ssd_iops_max", diskCapability.ssd_iops_max);
            props.setInt("ssd_iops_min", diskCapability.ssd_iops_min);
            props.setDouble("ssd_capacity", diskCapability.ssd_capacity);
            props.setInt("ssd_latency_max", diskCapability.ssd_latency_max);
            props.setInt("ssd_latency_min", diskCapability.ssd_latency_min);
            props.setInt("disk_type", diskCapability.disk_type);
        }

        void PlatformManager::determineDiskCapability()
        {
            diskCapability.disk_iops_max    = 100000;
            diskCapability.disk_iops_min    = 4000;

            if (conf->get<bool>("testing.manual_nodecap",false))
            {
                diskCapability.disk_iops_max    = conf->get<int>("testing.disk_iops_max", 100000);
                diskCapability.disk_iops_min    = conf->get<int>("testing.disk_iops_min", 6000);
            }
            diskCapability.disk_capacity    = 0x7ffff;
            diskCapability.disk_latency_max = 1000000 / diskCapability.disk_iops_min;
            diskCapability.disk_latency_min = 1000000 / diskCapability.disk_iops_max;
            diskCapability.ssd_iops_max     = 300000;
            diskCapability.ssd_iops_min     = 1000;
            diskCapability.ssd_capacity     = 0x10000;
            diskCapability.ssd_latency_max  = 1000000 / diskCapability.ssd_iops_min;
            diskCapability.ssd_latency_min  = 1000000 / diskCapability.ssd_iops_max;
            diskCapability.disk_type        = FDS_DISK_SATA;
            db->setNodeDiskCapability(diskCapability);
        }

        bool PlatformManager::sendNodeCapabilityToOM()
        {
            fpi::FDSP_RegisterNodeTypePtr    pkt(new fpi::FDSP_RegisterNodeType);

            // populate diskCapabiilitye from DiskPlatMod data

            pkt->disk_info = diskCapability;

            if (conf->get<bool>("testing.manual_nodecap",false))
            {
                pkt->disk_info.disk_iops_min = conf->get<int>("testing.disk_iops_min", 6000);
                pkt->disk_info.disk_iops_max = conf->get<int>("testing.disk_iops_max", 100000);
            }
            // do the send

            return true;
        }

        fds_int64_t PlatformManager::getNodeUUID(fpi::FDSP_MgrIdType svcType)
        {
            ResourceUUID    uuid;
            uuid.uuid_set_type(nodeInfo.uuid, svcType);

            return uuid.uuid_get_val();
        }

        int PlatformManager::run()
        {
            sendNodeCapabilityToOM();

            std::ostringstream message;

            // message << "Prepparing to begin monitoring:  " << m_appPidMap.size() << " process(es)";

            for (auto &element : m_appPidMap)
            {
                message << element.first << ":" << element.second << ", ";
            }

            LOGDEBUG << message.str();

            while (1)
            {
LOGDEBUG << "NOT monitoring:  " << m_appPidMap.size() << " process(es)";
                sleep(66);   /* we'll do hotplug uevent thread in here */
            }

            return 0;
        }
    }  // namespace pm
}  // namespace fds
