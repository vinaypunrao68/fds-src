/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_DISK_COMMON_H_
#define SOURCE_PLATFORM_INCLUDE_DISK_COMMON_H_

#include <string>

namespace fds
{
    /**
     * Common info about a disk shared among the main obj and its partitions (e.g. sda,
     * sda1, sda2...).  The main obj is responsible to free it after freeing all child objs
     * representing partitions.
     */
    class DiskCommon
    {
        protected:
            friend class PmDiskObj;
            friend class PmDiskInventory;

            std::string        dsk_blk_path;
            fds_disk_type_t    dsk_type;
            fds_tier_type_e    dsk_tier;

        public:
            explicit DiskCommon(const std::string &blk);
            virtual ~DiskCommon();

            inline const std::string &dsk_get_blk_path()
            {
                return dsk_blk_path;
            }
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_DISK_COMMON_H_
