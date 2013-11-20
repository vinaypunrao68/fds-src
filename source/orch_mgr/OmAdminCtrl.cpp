
/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include "OmAdminCtrl.h"

#define  REPLICATION_FACTOR	3
#define  LOAD_FACTOR		0.5
#define  BURST_FACTOR 		0.3
using namespace std;
namespace fds {

  FdsAdminCtrl::FdsAdminCtrl(const std::string& om_prefix, fds_log* om_log)
  :parent_log(om_log) {
   /* init the disk  resource  variable */
    initDiskCapabilities();
  }

  FdsAdminCtrl::~FdsAdminCtrl() { 
  }
   

   void FdsAdminCtrl::addDiskCapacity(class NodeInfo& n_info) {

   	total_disk_iops_max += n_info.disk_iops_max;
   	total_disk_iops_min += n_info.disk_iops_min;
   	total_disk_capacity += n_info.disk_capacity;
   	disk_latency_max = n_info.disk_latency_max;
   	disk_latency_min = n_info.disk_latency_min;
   	total_ssd_iops_max += n_info.ssd_iops_max;
   	total_ssd_iops_min += n_info.ssd_iops_min;
   	total_ssd_capacity += n_info.ssd_capacity;
   	ssd_latency_max = n_info.ssd_latency_max;
   	ssd_latency_min = n_info.ssd_latency_min;

   	avail_disk_iops_max += n_info.disk_iops_max;
   	avail_disk_iops_min += n_info.disk_iops_min;
   	avail_disk_capacity += n_info.disk_capacity;
   	avail_ssd_iops_max += n_info.ssd_iops_max;
   	avail_ssd_iops_min += n_info.ssd_iops_min;
   	avail_ssd_capacity += n_info.ssd_capacity;
     
	FDS_PLOG_SEV(parent_log, fds::fds_log::notification) << "Total Disk Resources "
                     << "  Total Disk iops Max : " << total_disk_iops_max
                     << "  Total Disk iops Min : " << total_disk_iops_min
                     << "  Total Disk capacity : " << total_disk_capacity
                     << "  Disk latency Max: " << disk_latency_max
                     << "  Disk latency Min: " << disk_latency_min
                     << "  Total Ssd iops Max : " << total_ssd_iops_max
                     << "  Total Ssd iops Min: " << total_ssd_iops_min
                     << "  Total Ssd capacity : " << total_ssd_capacity
                     << "  Ssd latency Max: " << ssd_latency_max
                     << "  Ssd latency Min : " << ssd_latency_min
                     << "  Avail Disk capacity : " << avail_disk_capacity
                     << "  Avail Disk  iops max : " << avail_disk_iops_max
                     << "  Avail Disk  iops min : " << avail_disk_iops_min;

   }

   void FdsAdminCtrl::getAvailableDiskCapacity(FdspVolInfoPtr&  pVolInfo ) {  

   }


   void  FdsAdminCtrl::updateAvailableDiskCapacity(FdspVolInfoPtr&  pVolInfo) {  
   }

   int FdsAdminCtrl::updateAdminControlParams(VolumeInfo  *pVolInfo) {
    	VolumeDesc  *pVolDesc = pVolInfo->properties;

        /* release  the resources since volume is deleted */
   	 total_vol_iops_min -= pVolDesc->iops_min; 
   	 total_vol_iops_min -= pVolDesc->iops_max;  
	 total_vol_disk_cap -= pVolDesc->capacity;
   }
   

int FdsAdminCtrl::volAdminControl(VolumeInfo  *pVolInfo) {
  double iopc_subcluster = 0;
  double iopc_subcluster_result = 0;
  VolumeDesc  *pVolDesc = pVolInfo->properties;

  iopc_subcluster = (avail_disk_iops_max/REPLICATION_FACTOR);

  if ((total_vol_disk_cap + pVolDesc->capacity) > avail_disk_capacity) {
    FDS_PLOG_SEV(parent_log, fds::fds_log::error) << " Cluster is running out of disk capacity \n"
                         << "total volume disk  capacity:"
                         << total_vol_disk_cap;
    return -1;
  }

  FDS_PLOG(parent_log) << " inside   admin control iopc sub cluster: "
                       << iopc_subcluster  << "\n";

  /* check the resource availability, if not return Error  */
  if (((total_vol_iops_min + pVolDesc->iops_min) <= (iopc_subcluster * LOAD_FACTOR)) &&
      ((((total_vol_iops_min) + ((total_vol_iops_max - total_vol_iops_min) * BURST_FACTOR))) + \
       (pVolDesc->iops_min + ((pVolDesc->iops_max - pVolDesc->iops_min) * BURST_FACTOR)) <= \
       iopc_subcluster)) {
    total_vol_iops_min += pVolDesc->iops_min;
    total_vol_iops_max += pVolDesc->iops_max;
    total_vol_disk_cap += pVolDesc->capacity;

    FDS_PLOG_SEV(parent_log, fds::fds_log::notification) << "updated disk params disk-cap:" 
							 << avail_disk_capacity << ":: min:"  
							 << total_vol_iops_min << ":: max:" 
							 << total_vol_iops_max << "\n";
    FDS_PLOG(parent_log) << " admin control successful \n";
    return 0;
  } else  {
    FDS_PLOG_SEV(parent_log, fds::fds_log::error) << " Unable to create Volume,Running out of IOPS \n ";
    FDS_PLOG_SEV(parent_log, fds::fds_log::error) << "Available disk Capacity:" 
						  << avail_disk_capacity << "::Total min IOPS:"  
						  <<  total_vol_iops_min << ":: Total max IOPS:" 
						  << total_vol_iops_max << "\n";
    return -1;
  }

  return -1;
}
   

   void FdsAdminCtrl::initDiskCapabilities() {

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
}
