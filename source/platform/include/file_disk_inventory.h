/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_FILE_DISK_INVENTORY_H_
#define SOURCE_PLATFORM_INCLUDE_FILE_DISK_INVENTORY_H_

namespace fds
{
    /**
     * Use file to simulate a disk.
     */

    class FileDiskInventory : public PmDiskInventory
    {
        protected:
            ChainList     dsk_files;
            const char   *dsk_devdir;

        public:
            explicit FileDiskInventory(const char *dir);
            ~FileDiskInventory();

            virtual void dsk_dump_all();
            virtual void dsk_do_partition();
            virtual void dsk_admit_all();
            virtual void dsk_mount_all();
            virtual bool disk_read_label(DiskLabelMgr *mgr, bool creat);

        private:
            void dsk_file_create(const char *type, int count, ChainList *list);
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_FILE_DISK_INVENTORY_H_