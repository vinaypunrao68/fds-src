/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_DISK_LABEL_MGR_H_
#define SOURCE_PLATFORM_INCLUDE_DISK_LABEL_MGR_H_

namespace fds
{
    /**
     * Disk label manager to coordinate label quorum.
     */

    class DiskLabelMgr
    {
        protected:
            DiskLabel   *dl_master;
            ChainList    dl_labels;
            int          dl_total_disks;
            int          dl_valid_labels;
            fds_mutex    dl_mtx;

            /**
             * Temp construct to build a map file for PL to read the disk mapping.
             */
            std::ofstream   *dl_map;

        public:
            DiskLabelMgr();
            virtual ~DiskLabelMgr();

            DiskLabel *dsk_master_label_mtx();
            void dsk_read_label(PmDiskObj::pointer disk);

            void dsk_reconcile_label(bool dsk_need_simulation, NodeUuid node_uuid);
            bool dsk_rec_label_map(PmDiskObj::pointer disk, int idx);

            // Clear the list of disk labels
            void clear();
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_DISK_LABEL_MGR_H_
