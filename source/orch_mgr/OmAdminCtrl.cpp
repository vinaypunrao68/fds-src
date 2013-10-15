
/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include "OmAdminCtrl.h"

#define  REPLICATION_FACTOR	3
#define  LOAD_FACTOR		0.5
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
     
     FDS_PLOG(parent_log) << "Total Disk Resources "
                     << "  Total Disk iops Max : " << total_disk_iops_max
                     << "  Total Disk iops Min : " << total_disk_iops_min
                     << "  Total Disk capacity : " << total_disk_capacity
                     << "  Disk latency Max: " << disk_latency_max
                     << "  Disk latency Min: " << disk_latency_min
                     << "  Total Ssd iops Max : " << total_ssd_iops_max
                     << "  Total Ssd iops Min: " << total_ssd_iops_min
                     << "  Total Ssd capacity : " << total_ssd_capacity
                     << "  Ssd latency Max: " << ssd_latency_max
                     << "  Ssd latency Min : " << ssd_latency_min;

   }

   void FdsAdminCtrl::getAvailableDiskCapacity(class VolumeInfo  *pVolInfo ) {  

   }


   void FdsAdminCtrl::updatetAvailableDiskCapacity(class VolumeInfo  *pVolInfo ) {  
   }

   void FdsAdminCtrl::volAdminControl(class VolumeInfo  *pVolInfo) {

     /* check the resource availability, if not return Error  */
      
    
     /* if the resource  available  update the available resource  continue with Volume creation */

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
   }
}
