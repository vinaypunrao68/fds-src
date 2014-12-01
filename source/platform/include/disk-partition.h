/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_DISK_PARTITION_H_
#define SOURCE_PLATFORM_INCLUDE_DISK_PARTITION_H_

#include <vector>
#include <parted/parted.h>
#include <disk.h>

namespace fds
{

    class DiskPartitionMgr;

    /**
     * Handle jobs related to partition of a disk.
     */
    class DiskPartition
    {
        protected:
            friend class DiskPartitionMgr;

            ChainLink                 dsk_link;
            PmDiskObj::pointer        dsk_obj;
            DiskPartitionMgr         *dsk_mgr;
            PedDevice                *dsk_pdev;
            PedDisk                  *dsk_pe;

        public:
            DiskPartition();
            virtual ~DiskPartition();

            virtual void dsk_partition_init(DiskPartitionMgr *mgr, PmDiskObj::pointer disk);
            virtual void dsk_partition_check();
    };

    typedef enum
    {
        DISK_OP_NONE                    = 0,
        DISK_OP_CHECK_PARTITION         = 1,
        DISK_OP_DO_PARTITION            = 2,
        DISK_OP_MAX
    } disk_op_e;

    /**
     * Switch board to route the main iterator function to the designated function.
     */
    class DiskOp : public DiskObjIter
    {
        protected:
            disk_op_e                dsk_op;
            DiskPartitionMgr        *dsk_mgr;

        public:
            explicit DiskOp(disk_op_e op, DiskPartitionMgr *mgr);
            virtual ~DiskOp();

            virtual bool dsk_iter_fn(DiskObj::pointer curr);
    };

    /**
     * Handle jobs to classify new discovered disks and partition them.
     */
    class DiskPartitionMgr : public DiskObjIter
    {
        protected:
            ChainList                dsk_need_label;
            fds_mutex                dsk_mtx;

        public:
            DiskPartitionMgr();
            virtual ~DiskPartitionMgr();

            virtual bool dsk_iter_fn(DiskObj::pointer curr);
            virtual bool dsk_iter_fn(DiskObj::pointer curr, DiskObjIter *arg);
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_DISK_PARTITION_H_
