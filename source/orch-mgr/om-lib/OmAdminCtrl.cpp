/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <string>
#include <OmAdminCtrl.h>

#define REPLICATION_FACTOR     (4)

// TRUST Anna & Vinay on these values :)
#define LOAD_FACTOR            (0.9)
#define BURST_FACTOR           (0.3)

namespace fds {

FdsAdminCtrl::FdsAdminCtrl(const std::string& om_prefix)
        : num_nodes(0) {
    /* init the disk  resource  variable */
    initDiskCapabilities();
}

FdsAdminCtrl::~FdsAdminCtrl() {
}

void FdsAdminCtrl::addDiskCapacity(const fpi::FDSP_AnnounceDiskCapability *diskCaps)
{
    ++num_nodes;

    total_node_iops_max += diskCaps->node_iops_max;
    total_node_iops_min += diskCaps->node_iops_min;
    total_disk_capacity += diskCaps->disk_capacity;
    total_ssd_capacity  += diskCaps->ssd_capacity;

    avail_node_iops_max += diskCaps->node_iops_max;
    avail_node_iops_min += diskCaps->node_iops_min;
    avail_disk_capacity += diskCaps->disk_capacity;
    avail_ssd_capacity  += diskCaps->ssd_capacity;

    LOGDEBUG << "Total Disk Resources "
             << "\n  Total Node iops Max : " << total_node_iops_max
             << "\n  Total Node iops Min : " << total_node_iops_min
             << "\n  Total Disk capacity : " << total_disk_capacity
             << "\n  Total Ssd capacity  : " << total_ssd_capacity
             << "\n  Avail Disk capacity : " << avail_disk_capacity
             << "\n  Avail Disk  iops max: " << avail_node_iops_max
             << "\n  Avail Disk  iops min: " << avail_node_iops_min;
}

void FdsAdminCtrl::removeDiskCapacity(const fpi::FDSP_AnnounceDiskCapability *diskCaps)
{
    fds_verify(num_nodes > 0);
    --num_nodes;

    fds_verify(total_node_iops_max >= static_cast<fds_uint64_t>(diskCaps->node_iops_max));
    total_node_iops_max -= diskCaps->node_iops_max;
    total_node_iops_min -= diskCaps->node_iops_min;

    total_disk_capacity -= diskCaps->disk_capacity;
    total_ssd_capacity  -= diskCaps->ssd_capacity;

    avail_node_iops_max -= diskCaps->node_iops_max;
    avail_node_iops_min -= diskCaps->node_iops_min;
    avail_disk_capacity -= diskCaps->disk_capacity;
    avail_ssd_capacity  -= diskCaps->ssd_capacity;

    LOGDEBUG << "Total Disk Resources "
             << "\n  Total Node iops Max : " << total_node_iops_max
             << "\n  Total Node iops Min : " << total_node_iops_min
             << "\n  Total Disk capacity : " << total_disk_capacity
             << "\n  Total Ssd capacity  : " << total_ssd_capacity
             << "\n  Avail Disk capacity : " << avail_disk_capacity
             << "\n  Avail Disk  iops max: " << avail_node_iops_max
             << "\n  Avail Disk  iops min: " << avail_node_iops_min;
}

void FdsAdminCtrl::updateAdminControlParams(VolumeDesc  *pVolDesc)
{
    // quick hack to allow system volumes
    if (pVolDesc->isSystemVolume()) {

        LOGDEBUG << "system volume : "
                 << "[" << pVolDesc->name << "-" << pVolDesc->volUUID << "]"
                 << " admitted unconditionally";
        return;
    }
    
    /* release  the resources since volume is deleted */

    // remember that volume descriptor has capacity in MB
    // but disk and ssd capacity is in GB
    double vol_capacity_GB = pVolDesc->capacity / 1024;

    LOGDEBUG << "desc iops_assured   : " << pVolDesc->iops_assured
             << " desc iops_throttle : " << pVolDesc->iops_throttle
             << " desc capacity (MB) : " << pVolDesc->capacity
             << " total iops assured : " << total_vol_iops_assured
             << " total iops throttle: " << total_vol_iops_throttle
             << " total capacity (GB): " << total_vol_disk_cap_GB;
        
    fds_verify(pVolDesc->iops_assured <= total_vol_iops_assured);            
    fds_verify(pVolDesc->iops_throttle <= total_vol_iops_throttle);          
    fds_verify(vol_capacity_GB <= total_vol_disk_cap_GB);

    total_vol_iops_assured -= pVolDesc->iops_assured;
    total_vol_iops_throttle -= pVolDesc->iops_throttle;
    total_vol_disk_cap_GB -= vol_capacity_GB;
}

fds_uint64_t FdsAdminCtrl::getMaxIOPC() const {
    double replication_factor = REPLICATION_FACTOR;
    if (num_nodes == 0) return 0;

    if (replication_factor > num_nodes) {
        // we will access at most num_nodes nodes
        replication_factor = num_nodes;
    }
    fds_verify(replication_factor != 0);  // make sure REPLICATION_FACTOR > 0
    double max_iopc_subcluster = (avail_node_iops_max/replication_factor);
    return max_iopc_subcluster;
}

Error FdsAdminCtrl::volAdminControl(VolumeDesc  *pVolDesc)
{
    Error err(ERR_OK);
    double iopc_subcluster = 0;
    double max_iopc_subcluster = 0;
    double iopc_subcluster_result = 0;
    // remember that volume descriptor has capacity in MB
    // but disk and ssd capacity is in GB
    double vol_capacity_GB = pVolDesc->capacity / 1024;
    fds_uint32_t replication_factor = REPLICATION_FACTOR;

    // quick hack to allow system volumes
    if (pVolDesc->isSystemVolume()) {
        pVolDesc->iops_assured = 0;
        LOGDEBUG << "system volume : "
                 << "[" << pVolDesc->name << "-" << pVolDesc->volUUID << "]"
                 << " admitted unconditionally";
        return err;
    }

    fds_verify(replication_factor != 0);  // make sure REPLICATION_FACTOR > 0
    if (replication_factor > num_nodes) {
        // we will access at most num_nodes nodes
        replication_factor = num_nodes;
    }
    if (replication_factor == 0) {
        // if we are here, then num_nodes == 0 (whis is # of SMs)
        LOGERROR << "Cannot admit any volume if there are no SMs! "
                 << " Start at least on SM and try again!";
        return Error(ERR_VOL_ADMISSION_FAILED);
    }

    // using iops min for iopc of subcluster, which we will use
    // for min_iops admission; max iops is what AM will allow to SMs
    // for better utilization of system perf capacity. We are starting
    // with more pessimistic admission control on min_iops, once QoS is
    // more stable, we may admit on average case or whatever we will find
    // works best
    iopc_subcluster = (avail_node_iops_min/replication_factor);
    // TODO(Anna) I think max_iopc_subcluster should be calculated as
    // num_nodes * min(node_iops_min from all nodes) / replication_factor
    max_iopc_subcluster = (avail_node_iops_max/replication_factor);

    if (pVolDesc->iops_assured <= 0) {
        LOGDEBUG << "assured iops is zero";
        pVolDesc->iops_assured = 0;
    }
    
    LOGDEBUG << "new data "
             << "[iops.assured:" << pVolDesc->iops_assured << "] "
             << "[iops.throttle:" << pVolDesc->iops_throttle << "]";

    // Check max object size
    if ((pVolDesc->maxObjSizeInBytes < minVolObjSize) ||
        ((pVolDesc->maxObjSizeInBytes % minVolObjSize) != 0)) {
        // We expect the max object size to be at least some min size
        // and a multiple of that size
        LOGERROR << "Invalid maximum object size of " << pVolDesc->maxObjSizeInBytes
                 << ", the minimum size is " << minVolObjSize;
        return Error(ERR_VOL_ADMISSION_FAILED);
    }

    if ((pVolDesc->iops_throttle > 0) && 
        (pVolDesc->iops_assured > pVolDesc->iops_throttle)) {
        LOGERROR << " Cannot admit volume " << pVolDesc->name
                 << " -- iops_assured must be below iops_throttle";
        return Error(ERR_VOL_ADMISSION_FAILED);
    }

    /**
     * Allow thin provisioning, the commented out code below doesn't allow thin provisioning.
     *
     * if ((total_vol_disk_cap_GB + vol_capacity_GB) > avail_disk_capacity) {
     *    LOGERROR << " Cluster is running out of disk capacity \n"
     *             << " Volume's capacity (GB) " << vol_capacity_GB
     *             << "total volume disk  capacity (GB):" << total_vol_disk_cap_GB;
     *    return Error(ERR_VOL_ADMISSION_FAILED);
     * }
     */

    LOGNORMAL << *pVolDesc;
    LOGNORMAL << " iopc_subcluster: " << iopc_subcluster
              << " replication_factor: " << replication_factor
              << " iops_throttle: " << total_vol_iops_throttle
              << " iops_assured: " << total_vol_iops_assured
              << " loadfactor: " << LOAD_FACTOR
              << " BURST_FACTOR: " << BURST_FACTOR;

    /* check the resource availability, if not return Error  */
    auto projected_total_vol_iops_assured = total_vol_iops_assured + pVolDesc->iops_assured;
    auto avail_iops = iopc_subcluster * LOAD_FACTOR;
    auto current_burst_iops = total_vol_iops_assured
            + (total_vol_iops_assured - total_vol_iops_assured) * BURST_FACTOR;
    auto newvol_burst_iops = pVolDesc->iops_assured
            + (pVolDesc->iops_throttle - pVolDesc->iops_assured) * BURST_FACTOR;
    if (projected_total_vol_iops_assured <= avail_iops
            && (current_burst_iops + newvol_burst_iops <= max_iopc_subcluster)) {
        total_vol_iops_assured += pVolDesc->iops_assured;
        total_vol_iops_throttle += pVolDesc->iops_throttle;
        total_vol_disk_cap_GB += vol_capacity_GB;

        LOGNOTIFY << "updated disk params disk-cap:"
                  << avail_disk_capacity << ":: assured:"
                  << total_vol_iops_assured << ":: throttle:"
                  << total_vol_iops_throttle << "\n";
        LOGNORMAL << " admin control successful \n";
        return Error(ERR_OK);
    } else  {
        LOGERROR << " Unable to create Volume,Running out of IOPS";
        LOGERROR << "Available disk Capacity:" << avail_disk_capacity
                 << ":: Total assured IOPS:" << total_vol_iops_assured
                 << ":: Total throttle IOPS:" << total_vol_iops_throttle
                 << ":: Requested assured IOPS:" << pVolDesc->iops_assured
                 << ":: Requested throttle IOPS:" << pVolDesc->iops_throttle
                 << ":: System min IOPS:" << iopc_subcluster
                 << ":: System max IOPS:" << max_iopc_subcluster;
        return Error(ERR_VOL_ADMISSION_FAILED);
    }
    return err;
}

Error FdsAdminCtrl::checkVolModify(VolumeDesc *cur_desc, VolumeDesc *new_desc)
{
    Error err(ERR_OK);
    double cur_total_vol_disk_cap_GB = total_vol_disk_cap_GB;
    fds_int64_t cur_total_vol_iops_assured = total_vol_iops_assured;
    fds_int64_t cur_total_vol_iops_throttle = total_vol_iops_throttle;

    /* check modify by first subtracting current vol values and doing admin 
     * control based on new values */
    updateAdminControlParams(cur_desc);

    err = volAdminControl(new_desc);
    if (!err.ok()) {
        /* cannot admit volume -- revert to original values */
        total_vol_disk_cap_GB = cur_total_vol_disk_cap_GB;
        total_vol_iops_assured = cur_total_vol_iops_assured;
        total_vol_iops_throttle = cur_total_vol_iops_throttle;
        return err;
    }
    /* success */
    return err;
}

void FdsAdminCtrl::initDiskCapabilities()
{
    total_node_iops_max = 0;
    total_node_iops_min = 0;
    total_disk_capacity = 0;
    total_ssd_capacity = 0;

    /* Available  capacity */
    avail_node_iops_max = 0;
    avail_node_iops_min = 0;
    avail_disk_capacity = 0;
    avail_ssd_capacity = 0;
    /*  per volume capacity */
    total_vol_iops_assured = 0;
    total_vol_iops_throttle = 0;
    total_vol_disk_cap_GB = 0;
}

void FdsAdminCtrl::userQosToServiceQos(fpi::FDSP_VolumeDescType *voldesc,
                                       fpi::FDSP_MgrIdType svc_type) {
    fds_verify(voldesc != NULL);
    // currently only SM has actual translation
    if (svc_type == fpi::FDSP_STOR_MGR) {
        double replication_factor = REPLICATION_FACTOR;
        if (num_nodes == 0) return;  // for now just keep user qos policy

        if (replication_factor > num_nodes) {
            // we will access at most num_nodes nodes
            replication_factor = num_nodes;
        }
        fds_verify(replication_factor != 0);  // make sure REPLICATION_FACTOR > 0

        // translate assured_iops
        auto vol_assured_iops = voldesc->iops_assured;
        voldesc->iops_assured = vol_assured_iops * replication_factor / num_nodes;
        LOGNORMAL << "User assured_iops " << vol_assured_iops
                  << " SM volume's assured_iops " << voldesc->iops_assured;
    }
}

}  // namespace fds
