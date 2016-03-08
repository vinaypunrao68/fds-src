/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */

#include "util/Log.h"
#include "fds_process.h"
#include "file_system_table.h"

namespace fds
{
    extern FdsProcess *g_fdsprocess;

    static constexpr long MOUNT_OPTION_SKIP = -1;
    static constexpr long MOUNT_OPTION_FS   = -2;

    // Add options to this map if options are added in /etc/fstab
    MountOptionMap mountOptionMap = {
            {"noatime", MS_NOATIME} ,
            {"nodiratime", MS_NODIRATIME},
            {"noauto", MOUNT_OPTION_SKIP},  // skip this option when mounting
            {"discard", MOUNT_OPTION_FS}    // filesystem-specific option
    };


    FileSystemTable::FileSystemTable (std::string fstabFile) : m_tabFile (fstabFile)
    {
    }

    void FileSystemTable::loadTab()
    {
        FILE *tab;

        tab = setmntent (m_tabFile.c_str(), "r");

        if (nullptr == tab)
        {
            LOGERROR << "Unable to open '" << m_tabFile << "' due to errno:  " << errno << ".  Unable to mount any unmounted FDS file systems";
            return;
        }

        struct mntent *loadedTabEntry;

        while (nullptr != (loadedTabEntry = getmntent (tab)))
        {
            LOGDEBUG << "Processing *tab file entry:  " << loadedTabEntry->mnt_dir << " from " << m_tabFile;

            m_entries.emplace_back (*loadedTabEntry);
        }

        endmntent (tab);
    }

    bool FileSystemTable::findByMountPoint (const std::string mountPoint) const
    {

        for (auto vectIter = m_entries.begin(); vectIter != m_entries.end(); ++vectIter)
        {
            if (vectIter->m_mountPath == mountPoint)
            {
                return true;
            }
        }

        return false;
    }

    const struct fds::FileSystemTable::TabEntry *FileSystemTable::findByDeviceName (std::string const deviceName)
    {
        std::vector <struct TabEntry>::iterator vectIter = m_entries.begin();

        for (; vectIter != m_entries.end(); ++vectIter)
        {
            if (vectIter->m_deviceName == deviceName)
            {
                return &(*vectIter);
            }
        }

        return nullptr;
    }

    void FileSystemTable::findNoneMountedFileSystems (const FileSystemTable & mtab, std::vector <std::string> &fileSystemsToMount)
    {
        const std::string fdsRoot (g_fdsprocess->proc_fdsroot()->dir_fdsroot());

        auto vectIter = m_entries.begin();

        for (; vectIter != m_entries.end(); ++vectIter)
        {
            if (std::equal (fdsRoot.begin(), fdsRoot.end(), vectIter->m_mountPath.begin()))
            {
                if (mtab.findByMountPoint (vectIter->m_mountPath))
                {
                    LOGDEBUG << "Found mounted FDS file system:  " << vectIter->m_mountPath;
                    continue;
                }

                fileSystemsToMount.push_back (vectIter->m_deviceName);
            }
        }
    }

    unsigned long FileSystemTable::parseMountOptions(std::string mountOptions, std::string& fsMountOptions)
    {
        unsigned long flags = 0;
        std::vector<std::string> options;
        std::istringstream s(mountOptions);
        std::string option;
        LOGDEBUG << "Parsing mount options " << mountOptions.c_str();
        while (getline(s, option, ','))
        {
            long flag = fds::mountOptionMap[option];
            if (flag > 0)
            {
                LOGDEBUG << "Adding mount flag for option " << option;
                flags |= flag;
            }
            else if (flag == MOUNT_OPTION_SKIP)
            {
                LOGDEBUG << "Not using mount option " << option;
            }
            else if (flag == MOUNT_OPTION_FS)
            {
                LOGDEBUG << "Adding filesystem specific mount option " << option;
                if (fsMountOptions.empty())
                {
                    fsMountOptions.append(",");
                }
                fsMountOptions.append(option);
            }
            else
            {
                LOGWARN << "Unknown mount option " << option << ", ignoring";
            }
        }
        return flags;
    }
}  // namespace fds
