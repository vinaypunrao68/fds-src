/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_PLATFORM_DISK_OBJ_H_
#define SOURCE_PLATFORM_INCLUDE_PLATFORM_DISK_OBJ_H_

#include <string>

#include "disk_object.h"

namespace fds
{
    class DiskCapability;
    class DiskLabel;
    class DiskCommon;

    class PmDiskObj : public DiskObject
    {
        public:
            typedef boost::intrusive_ptr<PmDiskObj> pointer;
            typedef boost::intrusive_ptr<const PmDiskObj> const_ptr;

            PmDiskObj();
            virtual ~PmDiskObj();

            /**
             * Return HW device path from raw path.
             * @return: true if the raw path is block SD device; false otherwise.
             * @param blk : /devices/pci00:00/0000:00:0d.0/ata3/host2/target2:0:0/2:0:0:0/block/
             * @param dev : /dev/sda
             */
            static bool dsk_blk_dev_path(const char *raw, std::string &blk, std::string &dev);

            static inline PmDiskObj::pointer dsk_cast_ptr(DiskObject::pointer ptr)
            {
                return static_cast<PmDiskObj *>(get_pointer(ptr));
            }

            static inline PmDiskObj::pointer dsk_cast_ptr(Resource::pointer ptr)
            {
                return static_cast<PmDiskObj *>(get_pointer(ptr));
            }

            inline PmDiskObj::pointer dsk_get_parent()
            {
                return dsk_parent;
            }

            inline DiskLabel *dsk_xfer_label()
            {
                DiskLabel   *ret = dsk_label; dsk_label = NULL; return ret;
            }

            inline DiskCapability *dsk_xfer_capability()
            {
                DiskCapability   *ret = dsk_capability; dsk_capability = NULL; return ret;
            }

            inline fds_uint64_t dsk_capacity_gb()
            {
                return dsk_cap_gb;
            }

            /**
             * Setup the mount point where the device is mounted as the result of the discovery.
             */
            void dsk_set_mount_point(const char *mnt);

            const std::string &dsk_get_mount_point();

            /**
             * Read in disk capabilities.
             */
            void dsk_read_capability();

            /**
             * Read in disk label.  Generate uuid for the disk if we don't have valid label.
             */
            void dsk_read_uuid();

            /**
             * Raw sector read/write to support super block update.  Only done in parent disk.
             */
            virtual ssize_t dsk_read(void *buf, fds_uint32_t sector, int sect_cnt, bool use_new_superblock);
            virtual ssize_t dsk_write(bool sim, void *buf, fds_uint32_t sector, int sect_cnt, bool use_new_superblock);

            /**
             * Return the slice device matching with the dev_path (e.g. /dev/sda, /dev/sda1...)
             */
            virtual PmDiskObj::pointer dsk_get_info(const std::string &dev_path);

            /**
             * Update the disk obj during the discovery process.
             * @param dev (i) - NULL if the obj already has the udev device.
             * @param ref (i) - sibling or parent object.  May need to fixup the chain.
             * @param b_path (i) - common path to the block device (e.g /devices/pci...)
             * @param d_path (i) - the device path from dev (e.g. /dev/sda1...)
             */
            virtual void dsk_update_device(struct udev_device *dev, PmDiskObj::pointer ref,
                                           const std::string  &b_path, const std::string  &d_path);
            /**
             * Iterate the parent object for all sub devices attached to it.
             */
            virtual void dsk_dev_foreach(DiskObjIter *iter, bool parent_only = false);

        protected:
            friend class PmDiskInventory;
            friend std::ostream &operator<< (std::ostream &, fds::PmDiskObj::pointer obj);

            ChainLink             dsk_part_link;     /**< link to dsk_partition list. */
            ChainLink             dsk_disc_link;     /**< link to discovery list.     */
            ChainList             dsk_part_head;     /**< sda -> { sda1, sda2... }    */
                                                     /**< /dev/sda is in rs_name.     */
            std::string           dsk_mount_pt;      /**< PL's mount point.           */
            const char           *dsk_raw_path;
            int                   dsk_raw_plen;
            int                   dsk_part_idx;
            int                   dsk_part_cnt;
            fds_uint64_t          dsk_cap_gb;

            dev_t                 dsk_my_devno;
            PmDiskObj::pointer    dsk_parent;
            struct udev_device   *dsk_my_dev;
            DiskCommon           *dsk_common;
            DiskLabel            *dsk_label;
            DiskCapability       *dsk_capability;

        private:
            void dsk_fixup_family_links(PmDiskObj::pointer ref);
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_PLATFORM_DISK_OBJ_H_
