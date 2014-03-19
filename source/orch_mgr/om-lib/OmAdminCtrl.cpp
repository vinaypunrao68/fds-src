
/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <string>
#include <OmAdminCtrl.h>

#define REPLICATION_FACTOR     (3)
#define LOAD_FACTOR            (0.5)
#define BURST_FACTOR           (0.3)

namespace fds {

FdsAdminCtrl::FdsAdminCtrl(const std::string& om_prefix, fds_log* om_log)
        : parent_log(om_log) {
    /* init the disk  resource  variable */
    initDiskCapabilities();
}

FdsAdminCtrl::~FdsAdminCtrl() {
}

void FdsAdminCtrl::addDiskCapacity(const node_capability_t &n_info)
{
    total_disk_iops_max += n_info.disk_iops_max;
    total_disk_iops_min += n_info.disk_iops_min;
    total_disk_capacity += n_info.disk_capacity;
    disk_latency_max     = n_info.disk_latency_max;
    disk_latency_min     = n_info.disk_latency_min;
    total_ssd_iops_max  += n_info.ssd_iops_max;
    total_ssd_iops_min  += n_info.ssd_iops_min;
    total_ssd_capacity  += n_info.ssd_capacity;
    ssd_latency_max      = n_info.ssd_latency_max;
    ssd_latency_min      = n_info.ssd_latency_min;

    avail_disk_iops_max += n_info.disk_iops_max;
    avail_disk_iops_min += n_info.disk_iops_min;
    avail_disk_capacity += n_info.disk_capacity;
    avail_ssd_iops_max  += n_info.ssd_iops_max;
    avail_ssd_iops_min  += n_info.ssd_iops_min;
    avail_ssd_capacity  += n_info.ssd_capacity;

    LOGNOTIFY << "Total Disk Resources "
              << "\n  Total Disk iops Max : " << total_disk_iops_max
              << "\n  Total Disk iops Min : " << total_disk_iops_min
              << "\n  Total Disk capacity : " << total_disk_capacity
              << "\n  Disk latency Max: " << disk_latency_max
              << "\n  Disk latency Min: " << disk_latency_min
              << "\n  Total Ssd iops Max : " << total_ssd_iops_max
              << "\n  Total Ssd iops Min: " << total_ssd_iops_min
              << "\n  Total Ssd capacity : " << total_ssd_capacity
              << "\n  Ssd latency Max: " << ssd_latency_max
              << "\n  Ssd latency Min : " << ssd_latency_min
              << "\n  Avail Disk capacity : " << avail_disk_capacity
              << "\n  Avail Disk  iops max : " << avail_disk_iops_max
              << "\n  Avail Disk  iops min : " << avail_disk_iops_min;
}

void FdsAdminCtrl::removeDiskCapacity(const node_capability_t &n_info)
{
    total_disk_iops_max -= n_info.disk_iops_max;
    total_disk_iops_min -= n_info.disk_iops_min;
    total_disk_capacity -= n_info.disk_capacity;
    disk_latency_max     = n_info.disk_latency_max;
    disk_latency_min     = n_info.disk_latency_min;
    total_ssd_iops_max  -= n_info.ssd_iops_max;
    total_ssd_iops_min  -= n_info.ssd_iops_min;
    total_ssd_capacity  -= n_info.ssd_capacity;
    ssd_latency_max      = n_info.ssd_latency_max;
    ssd_latency_min      = n_info.ssd_latency_min;

    avail_disk_iops_max -= n_info.disk_iops_max;
    avail_disk_iops_min -= n_info.disk_iops_min;
    avail_disk_capacity -= n_info.disk_capacity;
    avail_ssd_iops_max  -= n_info.ssd_iops_max;
    avail_ssd_iops_min  -= n_info.ssd_iops_min;
    avail_ssd_capacity  -= n_info.ssd_capacity;

    LOGNOTIFY << "Total Disk Resources "
              << "\n  Total Disk iops Max : " << total_disk_iops_max
              << "\n  Total Disk iops Min : " << total_disk_iops_min
              << "\n  Total Disk capacity : " << total_disk_capacity
              << "\n  Disk latency Max: " << disk_latency_max
              << "\n  Disk latency Min: " << disk_latency_min
              << "\n  Total Ssd iops Max : " << total_ssd_iops_max
              << "\n  Total Ssd iops Min: " << total_ssd_iops_min
              << "\n  Total Ssd capacity : " << total_ssd_capacity
              << "\n  Ssd latency Max: " << ssd_latency_max
              << "\n  Ssd latency Min : " << ssd_latency_min
              << "\n  Avail Disk capacity : " << avail_disk_capacity
              << "\n  Avail Disk  iops max : " << avail_disk_iops_max
              << "\n  Avail Disk  iops min : " << avail_disk_iops_min;
}

void FdsAdminCtrl::getAvailableDiskCapacity(const FdspVolInfoPtr pVolInfo)
{
}

void FdsAdminCtrl::updateAvailableDiskCapacity(const FdspVolInfoPtr pVolInfo)
{
}

void FdsAdminCtrl::updateAdminControlParams(VolumeDesc  *pVolDesc)
{
    /* release  the resources since volume is deleted */

    // remember that volume descriptor has capacity in MB
    // but disk and ssd capacity is in GB
    double vol_capacity_GB = pVolDesc->capacity / 1024;

    LOGERROR << "desc iops_min: " << pVolDesc->iops_min
             << "desc iops_max: " << pVolDesc->iops_max
             << "desc capacity (MB): " << pVolDesc->capacity
             << "total iops min : " << total_vol_iops_min
             << "total iops max: " << total_vol_iops_max
             << "total capacity (GB): " << total_vol_disk_cap;
    fds_verify(pVolDesc->iops_min <= total_vol_iops_min);
    fds_verify(pVolDesc->iops_max <= total_vol_iops_max);
    fds_verify(vol_capacity_GB <= total_vol_disk_cap);

    total_vol_iops_min -= pVolDesc->iops_min;
    total_vol_iops_max -= pVolDesc->iops_max;
    total_vol_disk_cap -= vol_capacity_GB;
}

Error FdsAdminCtrl::volAdminControl(VolumeDesc  *pVolDesc)
{
    Error err(ERR_OK);
    double iopc_subcluster = 0;
    double iopc_subcluster_result = 0;
    // remember that volume descriptor has capacity in MB
    // but disk and ssd capacity is in GB
    double vol_capacity_GB = pVolDesc->capacity / 1024;

    if (pVolDesc->iops_min > pVolDesc->iops_max) {
        LOGERROR << " Cannot admit volume " << pVolDesc->name
                 << " -- iops_min must be below iops_max";
        return Error(ERR_VOL_ADMISSION_FAILED);
    }
    iopc_subcluster = (avail_disk_iops_max/REPLICATION_FACTOR);

    if ((total_vol_disk_cap + vol_capacity_GB) > avail_disk_capacity) {
        LOGERROR << " Cluster is running out of disk capacity \n"
                 << " Volume's capacity (GB) " << vol_capacity_GB
                 << "total volume disk  capacity (GB):" << total_vol_disk_cap;
        return Error(ERR_VOL_ADMISSION_FAILED);
    }

    LOGNORMAL << *pVolDesc;
    LOGNORMAL << " iopc_subcluster:" << iopc_subcluster
              << " iops_max: " << total_vol_iops_max
              << " iops_min: " << total_vol_iops_min
              << " loadfactor: " << LOAD_FACTOR
              << " BURST_FACTOR: " << BURST_FACTOR;

    /* check the resource availability, if not return Error  */
    if (((total_vol_iops_min + pVolDesc->iops_min) <= (iopc_subcluster * LOAD_FACTOR)) &&
        ((((total_vol_iops_min) +
           ((total_vol_iops_max - total_vol_iops_min) * BURST_FACTOR))) + \
         (pVolDesc->iops_min +
          ((pVolDesc->iops_max - pVolDesc->iops_min) * BURST_FACTOR)) <= \
         iopc_subcluster)) {
        total_vol_iops_min += pVolDesc->iops_min;
        total_vol_iops_max += pVolDesc->iops_max;
        total_vol_disk_cap += vol_capacity_GB;

        LOGNOTIFY << "updated disk params disk-cap:"
                  << avail_disk_capacity << ":: min:"
                  << total_vol_iops_min << ":: max:"
                  << total_vol_iops_max << "\n";
        LOGNORMAL << " admin control successful \n";
        return Error(ERR_OK);
    } else  {
        LOGERROR << " Unable to create Volume,Running out of IOPS";
        LOGERROR << "Available disk Capacity:"
                 << avail_disk_capacity << "::Total min IOPS:"
                 <<  total_vol_iops_min << ":: Total max IOPS:"
                 << total_vol_iops_max;
        return Error(ERR_VOL_ADMISSION_FAILED);
    }
    return err;
}

Error FdsAdminCtrl::checkVolModify(VolumeDesc *cur_desc, VolumeDesc *new_desc)
{
    Error err(ERR_OK);
    fds_uint64_t cur_total_vol_disk_cap = total_vol_disk_cap;
    fds_uint64_t cur_total_vol_iops_min = total_vol_iops_min;
    fds_uint64_t cur_total_vol_iops_max = total_vol_iops_max;

    /* check modify by first subtracting current vol values and doing admin 
     * control based on new values */
    updateAdminControlParams(cur_desc);

    err = volAdminControl(new_desc);
    if (!err.ok()) {
        /* cannot admit volume -- revert to original values */
        total_vol_disk_cap = cur_total_vol_disk_cap;
        total_vol_iops_min = cur_total_vol_iops_min;
        total_vol_iops_max = cur_total_vol_iops_max;
        return err;
    }
    /* success */
    return err;
}

void FdsAdminCtrl::initDiskCapabilities()
{
    total_disk_iops_max = 0;
    total_disk_iops_min = 0;
    total_disk_capacity = 0;
    disk_latency_max = 0;
    disk_latency_min = 0;
    total_ssd_iops_max = 0;
    total_ssd_iops_min = 0;
    total_ssd_capacity = 0;
    ssd_latency_max = 0;
    ssd_latency_min = 0;

    /* Available  capacity */
    avail_disk_iops_max = 0;
    avail_disk_iops_min = 0;
    avail_disk_capacity = 0;
    avail_ssd_iops_max = 0;
    avail_ssd_iops_min = 0;
    avail_ssd_capacity = 0;
    /*  per volume capacity */
    total_vol_iops_min = 0;
    total_vol_iops_max = 0;
    total_vol_disk_cap = 0;
}

}  // namespace fds
