/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_FILE_DISK_OBJ_H_
#define SOURCE_PLATFORM_INCLUDE_FILE_DISK_OBJ_H_

namespace fds
{
    class FileDiskObj : public PmDiskObj
    {
        protected:
            friend class FileDiskInventory;

            int           dsk_fd;
            const char   *dsk_dir;

        public:
            explicit FileDiskObj(const char *dir);
            ~FileDiskObj();

            virtual ssize_t dsk_read(void *buf, fds_uint32_t sector, int sect_cnt);
            virtual ssize_t dsk_write(bool sim, void *buf, fds_uint32_t sector, int sect_cnt);
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_FILE_DISK_OBJ_H_
