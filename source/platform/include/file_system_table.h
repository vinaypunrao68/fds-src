/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_FILE_SYSTEM_TABLE_H_
#define SOURCE_PLATFORM_INCLUDE_FILE_SYSTEM_TABLE_H_

extern "C"
{
    #include <mntent.h>
    #include <sys/mount.h>
}

#include <string>

namespace fds
{
    typedef std::unordered_map<std::string, long> MountOptionMap;

    class FileSystemTable
    {
        public:
            struct TabEntry
            {
                explicit TabEntry (struct mntent const &entry) : m_deviceName (entry.mnt_fsname), m_mountPath (entry.mnt_dir), m_fileSystemType (entry.mnt_type), m_mountOptions (entry.mnt_opts)
                {
                }

                const std::string m_deviceName;
                const std::string m_mountPath;
                const std::string m_fileSystemType;
                const std::string m_mountOptions;
            };

            explicit FileSystemTable (std::string const tabFile);
            ~FileSystemTable() = default;

            void loadTab();
            void findNoneMountedFileSystems (const FileSystemTable &mtab, std::vector <std::string> &fileSystemsToMount);
            const struct TabEntry *findByDeviceName (std::string const deviceName);
            static unsigned long parseMountOptions(std::string mountOptions, std::string& fsMountOptions);

        protected:

        private:
            const std::string m_tabFile;
            std::vector <struct TabEntry> m_entries;

            bool findByMountPoint (const std::string mountPoint) const;

            static MountOptionMap mountOptionMap;
    };

}  // namespace fds

#endif  /* SOURCE_INCLUDE_SHARED_FDS_MAGIC_H_  // NOLINT */
