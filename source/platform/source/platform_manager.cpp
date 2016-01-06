/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

extern "C"
{
    #include <sys/mount.h>
    #include <dirent.h>
    #include <sys/types.h>
    #include <sys/wait.h>
}

#include <uuid/uuid.h>

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

#include <fdsp/health_monitoring_api_types.h>

#include "fds_module_provider.h"
#include <net/SvcMgr.h>
#include <net/SvcRequestPool.h>

#include "platform/platform_manager.h"
#include "platform/disk_capabilities.h"

#include "file_system_table.h"

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

        PlatformManager::PlatformManager() : Module ("pm"), m_appPidMap(), m_autoRestartFailedProcesses (false), m_startupAuditComplete (false), m_nodeRedisKeyId (""), m_diskUuidToDeviceMap()
        {
        }

        int PlatformManager::mod_init (SysParams const *const param)
        {
            const uint16_t MAX_NAP_LENGTH = 60;

            fdsConfig = new FdsConfigAccessor (g_fdsprocess->get_conf_helper());
            rootDir = g_fdsprocess->proc_fdsroot()->dir_fdsroot();
            loadRedisKeyId();
            m_db = new kvstore::PlatformDB (m_nodeRedisKeyId, rootDir, fdsConfig->get <std::string> ("redis_host", "localhost"), fdsConfig->get <int> ("redis_port", 6379), 1);

            m_serviceFlapDetector = new FlapDetector (fdsConfig->get_abs <uint32_t> ("fds.pm.service_management.flap_count", 0), fdsConfig->get_abs <uint32_t> ("fds.pm.service_management.flap_timeout", 0));

            int napTime = 1;

            while (!m_db->isConnected())
            {
                if (napTime < MAX_NAP_LENGTH)
                {
                    napTime <<= 1;

                    if (napTime > MAX_NAP_LENGTH)
                    {
                        napTime = MAX_NAP_LENGTH;
                    }
                }

                LOGCRITICAL << "unable to talk to redis @ [" << fdsConfig->get<std::string> ("redis_host", "localhost") << ":" << fdsConfig->get <int> ("redis_port", 6379) << "], will retry in " << napTime << " seconds";

                sleep (napTime);
            }

            m_db->getNodeInfo (m_nodeInfo);
            m_db->getNodeDiskCapability (diskCapability);

            if (m_nodeInfo.uuid <= 0)
            {
                NodeUuid    uuid (fds_get_uuid64 (get_uuid()));

                m_nodeInfo.uuid = uuid.uuid_get_val();
                m_nodeInfo.uuid = getNodeUUID (fpi::FDSP_PLATFORM);

                m_nodeInfo.fHasAm = false;
                m_nodeInfo.fHasDm = false;
                m_nodeInfo.fHasOm = false;
                m_nodeInfo.fHasSm = false;
                m_nodeInfo.bareAMPid = EMPTY_PID;
                m_nodeInfo.javaAMPid = EMPTY_PID;
                m_nodeInfo.dmPid = EMPTY_PID;
                m_nodeInfo.smPid = EMPTY_PID;

                // TODO (DJN) Set default state here

                LOGNOTIFY << "generated a new uuid for this node:  " << m_nodeInfo.uuid;
                m_db->setNodeInfo (m_nodeInfo);
            }
            else
            {
                LOGNOTIFY << "Using stored nodeInfo record for this node:  " << m_nodeInfo.uuid;
            }

            checkPidsDuringRestart();

            m_autoRestartFailedProcesses = fdsConfig->get_abs <bool> ("fds.feature_toggle.pm.restart_failed_children_processes");

            determineDiskCapability();

            if (loadDiskUuidToDeviceMap())
            {
                verifyAndMountFDSFileSystems();
            }

            loadEnvironmentVariables();

            return 0;
        }

        void PlatformManager::loadEnvironmentVariables()
        {
            const char SPACE = ' ';

            // Load the java_am Java Options
            std::string javaOptions ("");

            char *envValue = getenv ("XDI_JAVA_OPTS");

            if (NULL != envValue)
            {
                javaOptions = envValue;
            }
            else
            {
                javaOptions = util::strformat ("-Dfds.service.name=xdi -Dlog4j.configurationFile=%setc/log4j2.xml -Dfds-root=%s", rootDir.c_str(), rootDir.c_str());

            }
            LOGDEBUG << "XDI_JAVA_OPTS = ' " << javaOptions << " ' FDS-ROOT: " << rootDir;

            if (javaOptions.size() > 0)
            {
                std::istringstream options (javaOptions, std::istringstream::in);

                std::string token;

                while (options >> token)
                {
                    m_javaOptions.push_back (token);
                }
            }

            for (auto const &vectItem : m_javaOptions)
            {
                LOGDEBUG << "java_am command line option loaded:  '" << vectItem << "'";
            }

            // Load the java_am main class name
            envValue = getenv ("XDI_MAIN_CLASS");

            if (NULL == envValue)
            {
                m_javaXdiMainClassName = "com.formationds.am.Main";
            }
            else
            {
                m_javaXdiMainClassName = envValue;
            }

            // Load the Java Home directory for the am/xdi
            envValue = getenv ("XDI_JAVA_HOME");

            if (NULL == envValue)
            {
                LOGDEBUG << "XDI_JAVA_HOME is not defined.  Using java from PATH.";
                m_javaXdiJavaCmd = JAVA_PROCESS_NAME;
            }
            else
            {
                std::string jhome (envValue);
                LOGDEBUG << "Using XDI_JAVA_HOME=" << jhome;
                m_javaXdiJavaCmd = jhome + "/bin/" + JAVA_PROCESS_NAME;
            }
        }

        bool PlatformManager::loadDiskUuidToDeviceMap()
        {
            std::string const uuidToDeviceDirectory = "/dev/disk/by-uuid";

            DIR *directory = opendir (uuidToDeviceDirectory.c_str());
            int enoHold = errno;

            if (nullptr == directory)
            {
                LOGERROR << "Unabled to open directory:  " << uuidToDeviceDirectory << ", " << strerror (enoHold) << "(errno=" << enoHold << ").  No FDS file systems will be mounted";
                return false;
            }

            int returnCode;
            struct dirent *entry;

            while (NULL != (entry = readdir (directory)))
            {
                if (DT_LNK == entry->d_type)    // Verify this entry is a link
                {
                    size_t const BUFFER_LENGTH = 256;
                    char buffer[BUFFER_LENGTH];

                    std::string pathToLink = uuidToDeviceDirectory + '/' + entry->d_name;

                    int bufferBytesFilled = readlink (pathToLink.c_str(), buffer, BUFFER_LENGTH);
                    enoHold = errno;

                    if (bufferBytesFilled < 0)
                    {
                        LOGERROR << "Unabled to dereference link:  " << entry->d_name << ", " << strerror (enoHold) << "(errno=" << enoHold << ").  The file system referenced by " << entry->d_name << " will not be mounted (if mounting was desired)";
                        continue;
                    }

                    buffer[bufferBytesFilled] = '\0';          // readlink does not append a NULL

                    std::string deviceName = "/dev";
                    deviceName += strrchr (buffer, '/');

                    m_diskUuidToDeviceMap[entry->d_name] = deviceName;
                }
            }

            closedir (directory);
            return true;
        }

        void PlatformManager::checkPidsDuringRestart()
        {
            std::string procName;

            std::lock_guard <decltype (m_pidMapMutex)> lock (m_pidMapMutex);

            // This must come before the bare_am check, due to the possible stopProcess call
            if (m_nodeInfo.javaAMPid > 0)
            {
                procName = getProcName (JAVA_AM);

                if (procCheck (procName, m_nodeInfo.javaAMPid))
                {
                    m_appPidMap[procName] = m_nodeInfo.javaAMPid | PROC_CHECK_BITMASK;
                }
                else
                {
                    updateNodeInfoDbPid (JAVA_AM, EMPTY_PID);
                }
            }

            if (m_nodeInfo.bareAMPid > 0)
            {
                procName = getProcName (BARE_AM);

                if (procCheck (procName, m_nodeInfo.bareAMPid))
                {
                    m_appPidMap[procName] = m_nodeInfo.bareAMPid | PROC_CHECK_BITMASK;
                }
                else
                {
                    updateNodeInfoDbPid (BARE_AM, EMPTY_PID);
                    stopProcess (JAVA_AM);
                }
            }

            if (m_nodeInfo.dmPid > 0)
            {
                procName = getProcName (DATA_MANAGER);

                if (procCheck (procName, m_nodeInfo.dmPid))
                {
                    m_appPidMap[procName] = m_nodeInfo.dmPid | PROC_CHECK_BITMASK;
                }
                else
                {
                    updateNodeInfoDbPid (DATA_MANAGER, EMPTY_PID);
                }
            }

            if (m_nodeInfo.smPid > 0)
            {
                procName = getProcName (STORAGE_MANAGER);

                if (procCheck (procName, m_nodeInfo.smPid))
                {
                    m_appPidMap[procName] = m_nodeInfo.smPid | PROC_CHECK_BITMASK;
                }
                else
                {
                    updateNodeInfoDbPid (STORAGE_MANAGER, EMPTY_PID);
                }
            }
            m_startupAuditComplete = true;
        }

        //
        void PlatformManager::verifyAndMountFDSFileSystems()
        {
            std::vector <std::string> fileSystemsToMount;

            fds::FileSystemTable fstab ("/etc/fstab");
            fstab.loadTab();

            fds::FileSystemTable mtab ("/etc/mtab");
            mtab.loadTab();

            // Uinsg the fstab and the mtab, determine which file systems need to be mounted
            fstab.findNoneMountedFileSystems (mtab, fileSystemsToMount);

            if (fileSystemsToMount.size() > 0)
            {
                for (auto const &vectItem : fileSystemsToMount)
                {
                    std::string realDeviceName ("");
                    auto tabEntry = fstab.findByDeviceName (vectItem);

                    if (nullptr != tabEntry)
                    {
                        LOGNORMAL << "Mounting FDS File System:  " << vectItem;

                        FdsRootDir::fds_mkdir (tabEntry->m_mountPath.c_str());      // Create the mount point

                        std::string uuid = tabEntry->m_deviceName.substr (5);

                        auto item = m_diskUuidToDeviceMap.find (uuid);

                        if (m_diskUuidToDeviceMap.end() == item)
                        {
                            LOGERROR << "Unable to the hardware path for partition with UUID:  " << uuid << ", as found in fstab";
                            continue;
                        }

                        realDeviceName = item->second;

                        auto retCode = mount (realDeviceName.c_str(), tabEntry->m_mountPath.c_str(), tabEntry->m_fileSystemType.c_str(), 0, nullptr);

                        if (0 != retCode)
                        {
                            LOGERROR << "Failure mounting file system " << uuid << " with errno=" << errno;
                            continue;
                        }

                        // Meh, this should be a function TODO(donavan)
                        // add the entry to the mtab
                        auto tab = setmntent ("/etc/mtab", "a");

                        if (nullptr == tab)
                        {
                            LOGERROR << "Unable to open '" << "/etc/mtab" << "' due to errno:  " << errno << ".  Unable to mount any unmounted FDS file systems";
                            return;
                        }

                        struct mntent entryToAddToMtab;

                        entryToAddToMtab.mnt_fsname = const_cast <char *> (realDeviceName.c_str());
                        entryToAddToMtab.mnt_dir = const_cast <char *> (tabEntry->m_mountPath.c_str());
                        entryToAddToMtab.mnt_type = const_cast <char *> (tabEntry->m_fileSystemType.c_str());
                        entryToAddToMtab.mnt_opts = "rw";
                        entryToAddToMtab.mnt_freq = 0;
                        entryToAddToMtab.mnt_passno = 0;

                        if (addmntent (tab, &entryToAddToMtab) != 0)
                        {
                            LOGERROR << "Unabled to add entry to /etc/mtab for " << entryToAddToMtab.mnt_fsname;
                            // TODO(donavan), probably need to deal with an umount for the failure to add this FS to the mtab
                        }

                        endmntent (tab);
                    }
                    else
                    {
                        LOGNOTIFY << "Unable to find:  " << vectItem << " in fstab";
                    }
                }
            }
        }

        /*
         *  Load the key to the redis DB contents for this node or generate a new key and store is on the disk.
         *
         */
        void PlatformManager::loadRedisKeyId()
        {
            std::string const hostRedisKeyFilename = rootDir + "/var/.redis.id";

            std::ifstream redisKeyFile (hostRedisKeyFilename, std::ifstream::in);

            char redisUuidStr[64];

            if (redisKeyFile.fail())
            {
                uuid_t redisUuid;

                uuid_generate (redisUuid);
                uuid_unparse (redisUuid, redisUuidStr);

                std::ofstream redisKeyFileWrite (hostRedisKeyFilename, std::ifstream::out);

                if (redisKeyFileWrite.fail())
                {
                    LOGERROR << "Failed to open: '" << hostRedisKeyFilename << "' for write access.  Going to use the RedisKeyId of '" << redisUuidStr << "' for this platform instance.";
                }
                else
                {
                    LOGNORMAL << "Created " << hostRedisKeyFilename << " with " << redisUuidStr;
                    redisKeyFileWrite << redisUuidStr;
                    redisKeyFileWrite.close();
                }
            }
            else
            {
                redisKeyFile >> redisUuidStr;
                redisKeyFile.close();
                LOGNORMAL << "Loaded redisUUID of " << redisUuidStr << " from " << hostRedisKeyFilename;
            }

            m_nodeRedisKeyId = redisUuidStr;
        }

        bool PlatformManager::procCheck (std::string const procName, pid_t pid)
        {
           std::ostringstream procCommFilename;
           procCommFilename << "/proc/" << pid << "/comm";

           std::ifstream commandNameFile (procCommFilename.str(), std::ifstream::in);

           if (commandNameFile.fail())
           {
               LOGWARN "Looking for pid " << pid << " and it is gone.";
               return false;
           }

           std::string commandName;

           commandNameFile >> commandName;

           // If the process namne we are looking for (procName) does NOT equal the command name found, do additional verification, as java processes are an exception.
           // Should they be equal, our work is done, no else clause is needed and we call out to the return true
           if (procName != commandName)
           {
               // Now check for java and com.formationds.am.Main
               if (JAVA_PROCESS_NAME == commandName || m_javaXdiJavaCmd == commandName )
               {
                   std::ostringstream procCommandLineFileName;
                   procCommandLineFileName << "/proc/" << pid << "/cmdline";

                   std::ifstream commandLineFile (procCommandLineFileName.str(), std::ifstream::in);

                   if (commandLineFile.fail())
                   {
                       LOGWARN "Looking for java pid " << pid << " and it is gone.";
                       return false;
                   }

                   std::string arg;

                   // The contents of cmdline are null separated, this seems to always read the full line in the file.
                   // There might be a case where a while is needed.
                   commandLineFile >> arg;

                   // If the java class name was NOT found in the command line arguments
                   // Otherwise we fall through to the return true
                   if (std::string::npos == arg.find (procName))
                   {
                       // TODO (donavan) Need a decent way to test this...
                       LOGWARN "Looking for java pid " << pid << " and it is no longer " << procName;
                       return false;
                   }
               }
               else
               {
                   LOGWARN "Looking for process with pid " << pid << " and it is no longer " << procName;
                   return false;
               }
           }

           return true;
        }

        std::string PlatformManager::getProcName (int const procIndex)
        {
            try
            {
                return (m_idToAppNameMap.at (procIndex));
            }
            catch (const std::out_of_range &error)
            {
                // consider making this an assert or some kind of fatal error, this should never happen in a release type build
                LOGERROR << "PlatformManager::getProcName is unable to identify a textual process name for index value " << procIndex << ", " << error.what();
            }

            return "";
        }

        void PlatformManager::startProcess (int procIndex)
        {
            std::vector<std::string>    args;
            pid_t                       pid;
            std::string                 command;
            std::string                 procName = getProcName (procIndex);

            if (procName.empty())
            {
                return;         // Note, error logged in getProcName()
            }

            std::lock_guard <decltype (m_pidMapMutex)> lock (m_pidMapMutex);

            auto mapIter = m_appPidMap.find (procName);

            if (m_appPidMap.end() != mapIter)
            {
                LOGNORMAL << "Received a request to start " << procName << ", but it is already running.  Not doing anything.";
                return;
            }

            if (m_serviceFlapDetector->isServiceFlapping (procIndex))
            {
                // Flap detector handles error logging.
                notifyOmServiceStateChange (procIndex, 0, fpi::HealthState::HEALTH_STATE_FLAPPING_DETECTED_EXIT, "is flapping, PM will not auto restart (until another start service is requested by the OM).");
                return;
            }

            if (JAVA_AM == procIndex)
            {
                command = m_javaXdiJavaCmd;

                for (auto const &vectItem : m_javaOptions)
                {
                    args.push_back (vectItem);
                }

                args.push_back ("-classpath");
                args.push_back (rootDir+JAVA_CLASSPATH_OPTIONS);
//                args.push_back ("-Dfds.service.name=xdi");
//                args.push_back ("-Dlog4j.configurationFile=" + rootDir + "etc/log4j2.xml");

#ifdef DEBUG
                std::ostringstream remoteDebugger;
                remoteDebugger << JAVA_DEBUGGER_OPTIONS << fdsConfig->get <int> ("platform_port") + 7777;
                args.push_back (remoteDebugger.str());
#endif // DEBUG

                args.push_back (JAVA_AM_CLASS_NAME);
            }
            else
            {
                command = procName;
                args.push_back ("--foreground");
            }

            // Common command line options
            args.push_back (util::strformat ("--fds.pm.platform_uuid=%lld", m_nodeInfo.uuid));
            args.push_back (util::strformat ("--fds.common.om_ip_list=%s", fdsConfig->get_abs <std::string> ("fds.common.om_ip_list").c_str()));
            args.push_back (util::strformat ("--fds.pm.platform_port=%d", fdsConfig->get <int> ("platform_port")));

            pid = fds_spawn_service (command, rootDir, args, false);

            if (pid > 0)
            {
                LOGNORMAL << procName << " started by platformd as pid " << pid;
                m_appPidMap[procName] = pid;
                updateNodeInfoDbPid (procIndex, pid);
                updateNodeInfoDbState (procIndex, fpi::SERVICE_RUNNING);
            }
            else
            {
                LOGERROR << "fds_spawn_service() for " << procName << " FAILED to start by platformd with errno=" << errno;
            }
        }

        void PlatformManager::updateNodeInfoDbPid (int processType, pid_t pid)
        {
            switch (processType)
            {
                case BARE_AM:
                {
                    m_nodeInfo.bareAMPid = pid;

                } break;

                case JAVA_AM:
                {
                    m_nodeInfo.javaAMPid = pid;

                } break;

                case DATA_MANAGER:
                {
                    m_nodeInfo.dmPid = pid;

                } break;

                case STORAGE_MANAGER:
                {
                    m_nodeInfo.smPid = pid;

                } break;
            }

            m_db->setNodeInfo (m_nodeInfo);
        }

        void PlatformManager::updateNodeInfoDbState (int processType, fpi::pmServiceStateTypeId newState)
        {
            switch (processType)
            {
                case BARE_AM:
                {
                    m_nodeInfo.bareAMState = newState;

                } break;

                case JAVA_AM:
                {
                    m_nodeInfo.javaAMState = newState;

                } break;

                case DATA_MANAGER:
                {
                    m_nodeInfo.dmState = newState;

                } break;

                case STORAGE_MANAGER:
                {
                    m_nodeInfo.smState = newState;

                } break;
            }

            m_db->setNodeInfo (m_nodeInfo);
        }

        bool PlatformManager::waitPid (pid_t const pid, uint64_t waitTimeoutNanoSeconds, bool monitoring)  // 1-%-9
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
                        LOGNORMAL << "pid " << pid << " exited normally with exit code " << WEXITSTATUS (status);
                    }
                    else if (WIFSIGNALED (status))
                    {
                        if (monitoring)
                        {
                            LOGERROR << "pid " << pid << " exited unexpectedly.";
                        }
                        else
                        {
                            LOGWARN << "pid " << pid << " exited via a signal (likely SIGTERM or SIGKILL during a shutdown sequence)";
                        }
                    }

                    return true;
                }

                usleep (WAIT_PID_SLEEP_TIMER_MICROSECONDS);
                clock_gettime (CLOCK_REALTIME, &timeNow);
            }
            while (timeNow.tv_sec < endTime.tv_sec || (timeNow.tv_sec <= endTime.tv_sec && timeNow.tv_nsec < endTime.tv_nsec));

            return false;
        }

        void PlatformManager::stopProcess (int procIndex)
        {
            std::string    procName = getProcName (procIndex);

            if (procName.empty())
            {
                return;
            }

            std::map <std::string, pid_t>::iterator mapIter = m_appPidMap.find (procName);

            if (m_appPidMap.end() == mapIter)
            {
                LOGWARN << "Unable to find pid for " << procName << " in stopProcess()";
                return;
            }

            LOGNORMAL << "Preparing to stop " << procName << " via kill(pid, SIGTERM)";

            bool orphanChildProcess = mapIter->second & PROC_CHECK_BITMASK;
            int rc;

            pid_t pid = mapIter->second & ~PROC_CHECK_BITMASK;

            // TODO(DJN): check for pid < 2 here and error

            m_serviceFlapDetector->removeService (procIndex);

            if (orphanChildProcess)
            {
                rc = kill (pid, SIGKILL);

                if (rc < 0)
                {
                    LOGERROR << "Error sending signal (SIGKILL) to orphaned child process:  " << procName << "(pid = " << pid << ") errno = " << rc << "";
                }
            }
            else
            {
                rc = kill (pid, SIGTERM);

                if (rc < 0)
                {
                    LOGWARN << "Error sending signal (SIGTERM) to " << procName << "(pid = " << pid << ") errno = " << rc << ", will follow up with a SIGKILL";
                }

                // Wait for the SIGTERM to shutdown the process, otherwise revert to using SIGKILL
                if (rc < 0 || false == waitPid (pid, PROCESS_STOP_WAIT_PID_SLEEP_TIMER_NANOSECONDS))
                {
                    rc = kill (pid, SIGKILL);

                    if (rc < 0)
                    {
                        LOGERROR << "Error sending signal (SIGKILL) to " << procName << "(pid = " << pid << ") errno = " << rc << "";
                    }

                    waitPid (pid, PROCESS_STOP_WAIT_PID_SLEEP_TIMER_NANOSECONDS);

                    if (rc < 0)
                    {
                        LOGERROR << "Error sending signal (SIGKILL) to " << procName << "(pid = " << pid << ") errno = " << errno << "";
                    }
                }
            }

            m_appPidMap.erase (mapIter);
            updateNodeInfoDbPid (procIndex, EMPTY_PID);
        }

        // plf_start_node_services
        // -----------------------
        //
        void PlatformManager::activateServices (const fpi::ActivateServicesMsgPtr &activateMsg)
        {
            auto &info = activateMsg->info;

            while (false == m_startupAuditComplete)
            {
                usleep (100000);         // Delay activation until restarting has audited existing processes.
            }

            if (info.has_sm_service)
            {
                {
                    std::lock_guard <decltype (m_startQueueMutex)> lock (m_startQueueMutex);
                    m_startQueue.push_back (STORAGE_MANAGER);
                }

                m_startQueueCondition.notify_one();
                m_nodeInfo.fHasSm = true;
            }

            if (info.has_dm_service)
            {
                {
                    std::lock_guard <decltype (m_startQueueMutex)> lock (m_startQueueMutex);
                    m_startQueue.push_back (DATA_MANAGER);
                }

                m_startQueueCondition.notify_one();
                m_nodeInfo.fHasDm = true;
            }

            if (info.has_am_service)
            {
                {
                    std::lock_guard <decltype (m_startQueueMutex)> lock (m_startQueueMutex);
                    m_startQueue.push_back (BARE_AM);
                    m_startQueue.push_back (JAVA_AM);
                }

                m_startQueueCondition.notify_one();
                m_nodeInfo.fHasAm = true;
            }
        }

        void PlatformManager::deactivateServices(const fpi::DeactivateServicesMsgPtr &deactivateMsg)
        {
            std::lock_guard <decltype (m_pidMapMutex)> lock (m_pidMapMutex);

            if (deactivateMsg->deactivate_am_svc && m_nodeInfo.fHasAm)
            {
                stopProcess(JAVA_AM);
                stopProcess(BARE_AM);
                m_nodeInfo.fHasAm = false;
            }

            if (deactivateMsg->deactivate_dm_svc && m_nodeInfo.fHasDm)
            {
                stopProcess(DATA_MANAGER);
                m_nodeInfo.fHasDm = false;
            }

            if (deactivateMsg->deactivate_sm_svc && m_nodeInfo.fHasSm)
            {
                stopProcess(STORAGE_MANAGER);
                m_nodeInfo.fHasSm = false;
            }
        }

        void PlatformManager::addService (fpi::NotifyAddServiceMsgPtr const &addServiceMsg)
        {
            auto serviceList = addServiceMsg->services;

            for (auto const &vectItem : serviceList)
            {
                LOGDEBUG << "received an add service for type:  " << vectItem.svc_type;

                switch (vectItem.svc_type)
                {
                    case fpi::FDSP_ACCESS_MGR:
                    {
                        if (fpi::SERVICE_NOT_PRESENT != m_nodeInfo.bareAMState && fpi::SERVICE_NOT_PRESENT != m_nodeInfo.javaAMState)
                        {
                            if (fpi::SERVICE_RUNNING == m_nodeInfo.bareAMState && fpi::SERVICE_RUNNING == m_nodeInfo.javaAMState)
                            {
                                LOGERROR << "Received an unexpected add service for the AM when the AM services are already running.";
                            }
                            else             // SERVICE_PRESENT
                            {
                                LOGDEBUG << "No operation performed, received an add services request for AM services, but they are already added.";
                            }
                        }

                        updateNodeInfoDbState (JAVA_AM, fpi::SERVICE_NOT_RUNNING);
                        updateNodeInfoDbState (BARE_AM, fpi::SERVICE_NOT_RUNNING);

                    } break;

                    case fpi::FDSP_DATA_MGR:
                    {
                        if (fpi::SERVICE_NOT_PRESENT != m_nodeInfo.dmState)
                        {
                            if (fpi::SERVICE_RUNNING == m_nodeInfo.dmState)
                            {
                                LOGERROR << "Received an unexpected add service request for the DM when the DM service is already running.";
                            }
                            else             // SERVICE_PRESENT
                            {
                                LOGDEBUG << "No operation performed, received an add service request for the DM service, but it is already added.";
                            }
                        }

                        updateNodeInfoDbState (DATA_MANAGER, fpi::SERVICE_NOT_RUNNING);

                    } break;

                    case fpi::FDSP_STOR_MGR:
                    {
                        if (fpi::SERVICE_NOT_PRESENT != m_nodeInfo.smState)
                        {
                            if (fpi::SERVICE_RUNNING == m_nodeInfo.smState)
                            {
                                LOGERROR << "Received an unexpected add service request for the SM when the SM service is already running.";
                            }
                            else             // SERVICE_PRESENT
                            {
                                LOGDEBUG << "No operation performed, received an add service request for the SM service, but it is already added.";
                            }
                        }

                        updateNodeInfoDbState (STORAGE_MANAGER, fpi::SERVICE_NOT_RUNNING);

                    } break;

                    default:
                    {
                        LOGWARN << "Received an unexpected service type of " << vectItem.svc_type << " during an add service request.";

                    } break;
                }
            }
        }

        void PlatformManager::removeService (fpi::NotifyRemoveServiceMsgPtr const &removeServiceMsg)
        {
            auto serviceList = removeServiceMsg->services;

            for (auto const &vectItem : serviceList)
            {
                LOGNORMAL << "received a remove service for type:  " << vectItem.svc_type;

                switch (vectItem.svc_type)
                {
                    case fpi::FDSP_ACCESS_MGR:
                    {
                        if (fpi::SERVICE_NOT_RUNNING != m_nodeInfo.bareAMState && fpi::SERVICE_NOT_RUNNING != m_nodeInfo.javaAMState)
                        {
                            if (fpi::SERVICE_RUNNING == m_nodeInfo.bareAMState || fpi::SERVICE_RUNNING == m_nodeInfo.javaAMState)
                            {
                                LOGERROR << "Received an unexpected remove service for the AM when the AM services are running.";
                            }
                            else        // SERVICE_NO_PRESENT
                            {
                                LOGDEBUG << "No operation performed, received a remove services request for AM services, but they are already disabled.";
                            }
                        }

                        updateNodeInfoDbState (JAVA_AM, fpi::SERVICE_NOT_PRESENT);
                        updateNodeInfoDbState (BARE_AM, fpi::SERVICE_NOT_PRESENT);

                    } break;

                    case fpi::FDSP_DATA_MGR:
                    {
                        if (fpi::SERVICE_NOT_RUNNING != m_nodeInfo.dmState)
                        {
                            if (fpi::SERVICE_RUNNING == m_nodeInfo.dmState)
                            {
                                LOGERROR << "Received an unexpected remove service request for the DM when the DM service is running.";
                            }
                            else        // SERVICE_NO_PRESENT
                            {
                                LOGDEBUG << "No operation performed, received a remove service request for the DM service, but it is already disabled.";
                            }
                        }

                        updateNodeInfoDbState (DATA_MANAGER, fpi::SERVICE_NOT_PRESENT);

                    } break;

                    case fpi::FDSP_STOR_MGR:
                    {
                        if (fpi::SERVICE_NOT_RUNNING != m_nodeInfo.smState)
                        {
                            if (fpi::SERVICE_RUNNING == m_nodeInfo.smState)
                            {
                                LOGERROR << "Received an unexpected remove service request for the SM when the SM service is running.";
                            }
                            else        // SERVICE_NO_PRESENT
                            {
                                LOGDEBUG << "No operation performed, received a remove service request for the SM service, but it is already disabled.";
                            }
                        }

                        updateNodeInfoDbState (STORAGE_MANAGER, fpi::SERVICE_NOT_PRESENT);

                    } break;

                    default:
                    {
                        LOGWARN << "Received an unexpected service type of " << vectItem.svc_type;

                    } break;
                }
            }
        }

        void PlatformManager::startService (fpi::NotifyStartServiceMsgPtr const &startServiceMsg)
        {
            auto serviceList = startServiceMsg->services;

            auto svcMgr = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
            auto request = svcMgr->newEPSvcRequest (MODULEPROVIDER()->getSvcMgr()->getOmSvcUuid());

            fpi::SvcStateChangeRespPtr message (new fpi::SvcStateChangeResp());
            message->pmSvcUuid.svc_uuid = m_nodeInfo.uuid;

            for (auto const &vectItem : serviceList)
            {
                LOGNORMAL << "received a start service for type:  " << vectItem.svc_type;

                switch (vectItem.svc_type)
                {
                    case fpi::FDSP_ACCESS_MGR:
                    {
                        FDS_ProtocolInterface::SvcChangeReqInfo amChangeInfo;
                        amChangeInfo.actionCode = fpi::STARTED;
                        amChangeInfo.svcType    = fpi::FDSP_ACCESS_MGR;

                        if (fpi::SERVICE_NOT_RUNNING != m_nodeInfo.bareAMState && fpi::SERVICE_NOT_RUNNING != m_nodeInfo.javaAMState)
                        {
                            if (fpi::SERVICE_NOT_PRESENT == m_nodeInfo.bareAMState || fpi::SERVICE_NOT_PRESENT == m_nodeInfo.javaAMState)
                            {
                                LOGERROR << "Received an unexpected start service request for the AM when the AM services are not expected to be started.";
                            }
                            else           // SERVICE_RUNNING
                            {
                                amChangeInfo.actionCode = fpi::NO_ACTION;
                                LOGDEBUG << "No operation performed, received a start services request for AM services, but they are already running.";
                            }
                        }

                        message->changeList.push_back (amChangeInfo);

                        std::lock_guard <decltype (m_startQueueMutex)> lock (m_startQueueMutex);
                        m_startQueue.push_back (BARE_AM);
                        m_startQueue.push_back (JAVA_AM);

                    } break;

                    case fpi::FDSP_DATA_MGR:
                    {
                        FDS_ProtocolInterface::SvcChangeReqInfo dmChangeInfo;
                        dmChangeInfo.actionCode = fpi::STARTED;
                        dmChangeInfo.svcType    = fpi::FDSP_DATA_MGR;

                        if (fpi::SERVICE_NOT_RUNNING != m_nodeInfo.dmState)
                        {
                            if (fpi::SERVICE_NOT_PRESENT == m_nodeInfo.dmState)
                            {
                                LOGERROR << "Received an unexpected start service request for the DM when the DM service is not expected to be started.";
                            }
                            else           // SERVICE_RUNNING
                            {
                                dmChangeInfo.actionCode = fpi::NO_ACTION;
                                LOGDEBUG << "No operation performed, received a start service request for the DM service, but it is already running.";
                            }
                        }


                        message->changeList.push_back (dmChangeInfo);

                        std::lock_guard <decltype (m_startQueueMutex)> lock (m_startQueueMutex);
                        m_startQueue.push_back (DATA_MANAGER);

                    } break;

                    case fpi::FDSP_STOR_MGR:
                    {
                        FDS_ProtocolInterface::SvcChangeReqInfo smChangeInfo;
                        smChangeInfo.actionCode = fpi::STARTED;
                        smChangeInfo.svcType    = fpi::FDSP_STOR_MGR;

                        if (fpi::SERVICE_NOT_RUNNING != m_nodeInfo.smState)
                        {
                            if (fpi::SERVICE_NOT_PRESENT == m_nodeInfo.smState)
                            {
                                LOGERROR << "Received an unexpected start service request for the SM when the SM service is not expected to be started.";
                            }
                            else           // SERVICE_RUNNING
                            {
                                smChangeInfo.actionCode = fpi::NO_ACTION;
                                LOGDEBUG << "No operation performed, received a start service request for the SM service, but it is already running.";
                            }
                        }

                        message->changeList.push_back (smChangeInfo);


                        std::lock_guard <decltype (m_startQueueMutex)> lock (m_startQueueMutex);
                        m_startQueue.push_back (STORAGE_MANAGER);

                    } break;

                    default:
                    {
                        LOGWARN << "Received an unexpected service type of " << vectItem.svc_type;

                    } break;
                }
            }

            request->setPayload (FDSP_MSG_TYPEID (fpi::SvcStateChangeResp), message);
            request->invoke();

            m_startQueueCondition.notify_one();
        }

        void PlatformManager::stopService (fpi::NotifyStopServiceMsgPtr const &stopServiceMsg)
        {
            auto serviceList = stopServiceMsg->services;

            for (auto const &vectItem : serviceList)
            {
                LOGNORMAL << "received a stop service for type:  " << vectItem.svc_type;

                switch (vectItem.svc_type)
                {
                    case fpi::FDSP_ACCESS_MGR:
                    {
                        if (fpi::SERVICE_RUNNING != m_nodeInfo.bareAMState && fpi::SERVICE_RUNNING != m_nodeInfo.javaAMState)
                        {
                            if (fpi::SERVICE_NOT_PRESENT == m_nodeInfo.bareAMState || fpi::SERVICE_NOT_PRESENT == m_nodeInfo.javaAMState)
                            {
                                LOGERROR << "Received an unexpected stop service request for the AM when the AM services are not expected to be started.";
                            }
                            else            // SERVICE_NOT_RUNNING
                            {
                                LOGDEBUG << "No operation performed, received a stop services request for AM services, but they are already stopped.";
                            }
                        }

                        std::lock_guard <decltype (m_pidMapMutex)> lock (m_pidMapMutex);
                        stopProcess (JAVA_AM);
                        updateNodeInfoDbState (JAVA_AM, fpi::SERVICE_NOT_RUNNING);
                        stopProcess (BARE_AM);
                        updateNodeInfoDbState (BARE_AM, fpi::SERVICE_NOT_RUNNING);

                    } break;

                    case fpi::FDSP_DATA_MGR:
                    {
                        if (fpi::SERVICE_RUNNING != m_nodeInfo.dmState)
                        {
                            if (fpi::SERVICE_NOT_PRESENT == m_nodeInfo.dmState)
                            {
                                LOGERROR << "Received an unexpected start service request for the DM when the DM service is not expected to be started.";
                            }
                            else            // SERVICE_NOT_RUNNING
                            {
                                LOGDEBUG << "No operation performed, received a stop service request for the DM service, but it is already stopped.";
                            }
                        }

                        std::lock_guard <decltype (m_pidMapMutex)> lock (m_pidMapMutex);
                        stopProcess (DATA_MANAGER);
                        updateNodeInfoDbState (DATA_MANAGER, fpi::SERVICE_NOT_RUNNING);

                    } break;

                    case fpi::FDSP_STOR_MGR:
                    {
                        if (fpi::SERVICE_RUNNING != m_nodeInfo.smState)
                        {
                            if (fpi::SERVICE_NOT_PRESENT == m_nodeInfo.smState)
                            {
                                LOGERROR << "Received an unexpected stop service request for the SM when the SM service is not expected to be started.";
                            }
                            else            // SERVICE_NOT_RUNNING
                            {
                                LOGDEBUG << "No operation performed, received a stop service request for the SM service, but it is already stopped.";
                            }
                        }

                        std::lock_guard <decltype (m_pidMapMutex)> lock (m_pidMapMutex);
                        stopProcess (STORAGE_MANAGER);
                        updateNodeInfoDbState (STORAGE_MANAGER, fpi::SERVICE_NOT_RUNNING);

                    } break;

                    default:
                    {
                        LOGWARN << "Received an unexpected service type of " << vectItem.svc_type;

                    } break;
                }
            }
        }

        void PlatformManager::heartbeatCheck (fpi::HeartbeatMessagePtr const &heartbeatMsg)
        {
            LOGDEBUG << "Sending heartbeatMessage ack from PM uuid: " << std::hex << heartbeatMsg->svcUuid.uuid << std::dec;

            auto svcMgr = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
            auto request = svcMgr->newEPSvcRequest (MODULEPROVIDER()->getSvcMgr()->getOmSvcUuid());

            request->setPayload (FDSP_MSG_TYPEID (fpi::HeartbeatMessage), heartbeatMsg);
            request->invoke();
        }

        void PlatformManager::updateServiceInfoProperties (std::map<std::string, std::string> *data)
        {
            determineDiskCapability();
            util::Properties props = util::Properties (data);
            props.set ("fds_root", rootDir);
            props.setInt ("uuid", m_nodeInfo.uuid);
            props.setInt ("node_iops_max", diskCapability.node_iops_max);
            props.setInt ("node_iops_min", diskCapability.node_iops_min);
            props.setDouble ("disk_capacity", diskCapability.disk_capacity);
            props.setDouble ("ssd_capacity", diskCapability.ssd_capacity);
            props.setInt ("disk_type", diskCapability.disk_type);
        }

        // TODO: this needs to populate real data from the disk module labels etc.
        // it may want to load the value from the database and validate it against
        // DiskPlatModule data, or just load from the DiskPlatModule and be done
        // with it.  Depends somewhat on how expensive it is to traverse the DiskPlatModule
        // and calculate all the data.
        void PlatformManager::determineDiskCapability()
        {
            auto ssd_iops_max = fdsConfig->get<uint32_t> ("capabilities.disk.ssd.iops_max");
            auto ssd_iops_min = fdsConfig->get<uint32_t> ("capabilities.disk.ssd.iops_min");
            auto hdd_iops_max = fdsConfig->get<uint32_t> ("capabilities.disk.hdd.iops_max");
            auto hdd_iops_min = fdsConfig->get<uint32_t> ("capabilities.disk.hdd.iops_min");
            auto space_reserve = fdsConfig->get <float> ("capabilities.disk.reserved_space");

            DiskPlatModule* dpm = DiskPlatModule::dsk_plat_singleton();
            auto disk_counts = dpm->disk_counts();

            if (0 == (disk_counts.first + disk_counts.second))
            {
                // We don't have real disks
                diskCapability.disk_capacity = 0x7ffff;
                diskCapability.ssd_capacity = 0x10000;
            }
            else
            {

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

            if (fdsConfig->get<bool> ("testing.manual_nodecap",false))
            {
                diskCapability.node_iops_max    = fdsConfig->get <int> ("testing.node_iops_max", 100000);
                diskCapability.node_iops_min    = fdsConfig->get <int> ("testing.node_iops_min", 6000);
            }

            LOGDEBUG << "Set node iops max to: " << diskCapability.node_iops_max;
            LOGDEBUG << "Set node iops min to: " << diskCapability.node_iops_min;

            m_db->setNodeDiskCapability (diskCapability);
        }

        fds_uint64_t PlatformManager::getNodeUUID (fpi::FDSP_MgrIdType svcType)
        {
            ResourceUUID    uuid;
            uuid.uuid_set_type (m_nodeInfo.uuid, svcType);

            return uuid.uuid_get_val();
        }

        void PlatformManager::startQueueMonitor()
        {
            LOGDEBUG << "Starting thread for PlatformManager::startQueueMonitor()";

            while (true)
            {
                std::unique_lock <decltype (m_startQueueMutex)> lock (m_startQueueMutex);
                m_startQueueCondition.wait (lock, [this] { return !m_startQueue.empty(); });

                while (!m_startQueue.empty())
                {
                    auto index = m_startQueue.front();
                    m_startQueue.pop_front();

                    startProcess (index);
                }
            }
        }

        void PlatformManager::childProcessMonitor()
        {
            LOGDEBUG << "Starting thread for PlatformManager::childProcessMonitor()";

#ifdef DEBUG
            uint32_t count = 0;
            uint32_t lastCount = 0;
#endif

            bool deadProcessesFound;

            while (true)
            {
                deadProcessesFound = false;

                {   // Create a context for the lock_guard
                    std::lock_guard <decltype (m_pidMapMutex)> lock (m_pidMapMutex);
#ifdef DEBUG
                    count = m_appPidMap.size();
                    if (count != lastCount)
                    {
                        LOGNORMAL << "Now monitoring " << count << " children (was " << lastCount << ")";
                        lastCount = count;
                    }
#endif
                    bool    orphanAlive;
                    bool    orphanChildProcess;
                    pid_t   pid;
                    std::string procName;

                    for (auto mapIter = m_appPidMap.begin(); m_appPidMap.end() != mapIter;)
                    {
                        orphanAlive = true;
                        orphanChildProcess = mapIter->second & PROC_CHECK_BITMASK;
                        pid = mapIter->second & ~PROC_CHECK_BITMASK;
                        procName = mapIter->first;

                        if (orphanChildProcess)
                        {
                            orphanAlive = procCheck (procName, pid);
                        }

                        if (!orphanAlive || waitPid (mapIter->second, 1000, true))
                        {

                            int appIndex = -1;

                            // Find the appIndex of the process
                            for (auto iter = m_idToAppNameMap.begin(); m_idToAppNameMap.end() != iter; iter++)
                            {
                                if (iter->second == procName)
                                {
                                    appIndex = iter->first;
                                    break;
                                }
                            }

                            if (BARE_AM == appIndex)
                            {
                                LOGWARN << "Discovered an exited bare_am process, also bringing down XDI";
                                stopProcess (JAVA_AM);
                            }

                            notifyOmServiceStateChange (appIndex, mapIter->second, fpi::HealthState::HEALTH_STATE_UNEXPECTED_EXIT, "unexpectedly exited");
                            m_appPidMap.erase (mapIter++);
                            updateNodeInfoDbPid (appIndex, EMPTY_PID);

                            if (m_autoRestartFailedProcesses)
                            {
                                {   // context for lock_guard
                                    deadProcessesFound = true;
                                    std::lock_guard <decltype (m_startQueueMutex)> lock (m_startQueueMutex);
                                    m_startQueue.push_back (appIndex);
                                }

                                if (BARE_AM == appIndex)
                                {
                                    std::lock_guard <decltype (m_startQueueMutex)> lock (m_startQueueMutex);
                                    m_startQueue.push_back (JAVA_AM);
                                }
                            }
                        }
                        else
                        {
                            ++mapIter;
                        }
                    }
                }  // lock_guard context

                if (deadProcessesFound)
                {
                    m_startQueueCondition.notify_one();
                }

                usleep (PROCESS_MONITOR_SLEEP_TIMER_MICROSECONDS);
            }
        }

        void PlatformManager::notifyOmServiceStateChange (int const appIndex, pid_t const procPid, FDS_ProtocolInterface::HealthState state, std::string const message)
        {
            std::string procName = getProcName (appIndex);

            std::vector <fpi::SvcInfo> serviceMap;
            MODULEPROVIDER()->getSvcMgr()->getSvcMap (serviceMap);

            fpi::SvcInfo const *serviceRecord = nullptr;

            fpi::FDSP_MgrIdType serviceType (fpi::FDSP_INVALID_SVC);

            switch (appIndex)
            {
                case BARE_AM:
                case JAVA_AM:
                {
                    serviceType = fpi::FDSP_ACCESS_MGR;

                } break;

                case DATA_MANAGER:
                {
                    serviceType = fpi::FDSP_DATA_MGR;

                } break;

                case STORAGE_MANAGER:
                {
                    serviceType = fpi::FDSP_STOR_MGR;

                } break;
            }

            // Search through the service map looking for the entity corresponding to this process
            for (auto const &vectItem : serviceMap)
            {
                ResourceUUID    uuid (vectItem.svc_id.svc_uuid.svc_uuid);

                // Check if this is a service on this node and is the same service type as the expired process
                if (getNodeUUID (fpi::FDSP_PLATFORM) == uuid.uuid_get_base_val() &&  uuid.uuid_get_type() == serviceType)
                {
                    serviceRecord = &vectItem;
                    break;
                }
            }

            if (nullptr == serviceRecord)
            {
                LOGERROR << "Unable to find a service map record for a process that platformd wished to send a Health Report on behalf of.";
                return;
            }

            std::ostringstream textualContent;
            textualContent << "Platform detected that " << procName << " (pid = " << procPid << ") " << message << ".";

            fpi::NotifyHealthReportPtr message (new fpi::NotifyHealthReport());

            message->healthReport.serviceInfo.svc_id.svc_uuid.svc_uuid = serviceRecord->svc_id.svc_uuid.svc_uuid;
            message->healthReport.serviceInfo.svc_id.svc_name = serviceRecord->name;
            message->healthReport.serviceInfo.svc_port = serviceRecord->svc_port;
            message->healthReport.platformUUID.svc_uuid.svc_uuid = m_nodeInfo.uuid;
            message->healthReport.serviceState = state;
            message->healthReport.statusCode = fds::PLATFORM_ERROR_UNEXPECTED_CHILD_DEATH;
            message->healthReport.statusInfo = textualContent.str();

            auto svcMgr = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
            auto request = svcMgr->newEPSvcRequest (MODULEPROVIDER()->getSvcMgr()->getOmSvcUuid());

            request->setPayload (FDSP_MSG_TYPEID (fpi::NotifyHealthReport), message);
            request->invoke();
        }

        void PlatformManager::notifyDiskMapChange ()
        {
            fpi::SvcUuid smUuid, dmUuid;
            std::vector <fpi::SvcInfo> serviceMap;
            MODULEPROVIDER()->getSvcMgr()->getSvcMap (serviceMap);
            // Find DM and SM on the service map
            for (auto const &vectItem : serviceMap)
            {
                fpi::SvcUuid svcUuid = vectItem.svc_id.svc_uuid;
                ResourceUUID    uuid (vectItem.svc_id.svc_uuid.svc_uuid);

                // Check if this is an SM/DM service on this node
                if (getNodeUUID (fpi::FDSP_PLATFORM) == uuid.uuid_get_base_val())
                {
                    if (smUuid.svc_uuid == 0 && vectItem.svc_type == fpi::FDSP_STOR_MGR)
                    {
                        LOGDEBUG << "Found local SM service " << smUuid;
                        smUuid = svcUuid;
                    }
                    else if (dmUuid.svc_uuid == 0 && vectItem.svc_type == fpi::FDSP_DATA_MGR)
                    {
                        LOGDEBUG << "Found local DM service " << dmUuid;
                        dmUuid = svcUuid;
                    }
                }
            }
            fpi::NotifyDiskMapChangePtr message (new fpi::NotifyDiskMapChange());

            auto svcMgr = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
            if (smUuid.svc_uuid != 0)
            {
                LOGNORMAL << "Notifying SM about a disk-map change";
                auto smRequest = svcMgr->newEPSvcRequest (smUuid);
                smRequest->setPayload (FDSP_MSG_TYPEID (fpi::NotifyDiskMapChange), message);
                smRequest->invoke();
            }
            if (dmUuid.svc_uuid != 0)
            {
                LOGNORMAL << "Notifying DM about a disk-map change";
                auto dmRequest = svcMgr->newEPSvcRequest (dmUuid);
                dmRequest->setPayload (FDSP_MSG_TYPEID (fpi::NotifyDiskMapChange), message);
                dmRequest->invoke();
            }
        }

        void PlatformManager::run()
        {
            std::thread startQueueMonitorThread (&PlatformManager::startQueueMonitor, this);
            startQueueMonitorThread.detach();

            std::thread childMonitorThread (&PlatformManager::childProcessMonitor, this);
            childMonitorThread.detach();

            DiskPlatModule* dpm = DiskPlatModule::dsk_plat_singleton();

            while (1)
            {
                dpm->dsk_monitor_hotplug();
                notifyDiskMapChange();
            }
        }
    }  // namespace pm
}  // namespace fds
