/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_PM_DISK_INVENTORY_H_
#define SOURCE_PLATFORM_INCLUDE_PM_DISK_INVENTORY_H_

#include <string>

#include "disk_inventory.h"
#include "platform_disk_obj.h"

namespace fds
{
    class DiskPartMgr;
    class DiskLabelMgr;
    class DiskCapabilitiesMgr;

    typedef std::unordered_map<std::string, PmDiskObj::pointer> DiskDevMap;

    class PmDiskInventory : public DiskInventory
    {
        protected:
            DiskDevMap    dsk_dev_map;
            uint32_t      dsk_qualify_cnt;   /**< disks that can run FDS SW.   */
            ChainList     dsk_discovery;
            ChainList     dsk_curr_inv;      /**< disks that were enumerated.  */
            DiskPartMgr   *dsk_partition;

            Resource *rs_new(const ResourceUUID &uuid);

        public:
            typedef boost::intrusive_ptr<PmDiskInventory> pointer;
            typedef boost::intrusive_ptr<const PmDiskInventory> const_ptr;

            PmDiskInventory();
            virtual ~PmDiskInventory();

            /**
             * Return true if the discovered inventory needs disk simulation.
             */
            bool dsk_need_simulation();

            virtual PmDiskObj::pointer dsk_get_info(const ResourceUUID &uuid)
            {
                return PmDiskObj::dsk_cast_ptr(rs_get_resource(uuid));
            }

            virtual PmDiskObj::pointer dsk_get_info(const char *name)
            {
                return PmDiskObj::dsk_cast_ptr(rs_get_resource(name));
            }

            virtual PmDiskObj::pointer dsk_get_info(dev_t dev_node);
            virtual PmDiskObj::pointer dsk_get_info(dev_t dev_node, int partition);
            virtual PmDiskObj::pointer dsk_get_info(const std::string &b);
            virtual PmDiskObj::pointer dsk_get_info(const std::string &b, const std::string &d);

            virtual void dsk_dump_all();
            virtual void dsk_discovery_begin();
            virtual void dsk_discovery_add(PmDiskObj::pointer disk, PmDiskObj::pointer ref);
            virtual void dsk_discovery_update(PmDiskObj::pointer disk);
            virtual void dsk_discovery_remove(PmDiskObj::pointer disk);
            virtual void dsk_discovery_done();

            virtual void dsk_do_partition();
            virtual void dsk_admit_all();
            virtual void dsk_mount_all();
            virtual bool disk_read_capabilities(DiskCapabilitiesMgr *mgr);
            virtual void disk_reconcile_label(DiskLabelMgr *mgr, NodeUuid node_uuid, fds_uint16_t& largest_disk_index);

            virtual void clear_inventory();
    };
}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_PM_DISK_INVENTORY_H_
