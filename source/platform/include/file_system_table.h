/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_FILE_SYSTEM_TABLE_H_
#define SOURCE_PLATFORM_INCLUDE_FILE_SYSTEM_TABLE_H_

#include <string>
#include <mntent.h>

namespace fds
{
    class FileSystemTable
    {
        public:
            struct TabEntry
            {
                explicit TabEntry (struct mntent const &entry) : m_deviceName (entry.mnt_fsname), m_mountPath (entry.mnt_dir), m_fileSystemType (entry.mnt_type), m_mountOptions (entry.mnt_opts)
                {
                }

                std::string m_deviceName;
                std::string m_mountPath;
                std::string m_fileSystemType;
                std::string m_mountOptions;
            };

            explicit FileSystemTable (std::string const tabFile);
            ~FileSystemTable();

            void loadTab();
            void findNoneMountedFileSystems (FileSystemTable &mtab, std::vector <std::string> &fileSystemsToMount);
            // struct TabEntry const &findByDeviceName (std::string deviceName);
            struct TabEntry *findByDeviceName (std::string deviceName);

        protected:

        private:
            std::string m_tabFile;
            std::vector <struct TabEntry> m_entries;

            bool findByMountPoint (std::string mountPoint);
    };

}  // namespace fds

#endif  /* SOURCE_INCLUDE_SHARED_FDS_MAGIC_H_  // NOLINT */
