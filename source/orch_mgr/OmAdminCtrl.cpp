
/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include "OmAdminCtrl.h"

namespace fds {

  FdsAdminCtrl::FdsAdminCtrl(const std::string& om_prefix, fds_log* om_log)
  :parent_log(om_log) {
   /* init the disk  resource  variable */
    initDiskCapabilities();
  }

  FdsAdminCtrl::~FdsAdminCtrl() { 
  }
   

   void FdsAdminCtrl::addDiskCapacity(class NodeInfo& n_info) {

   	total_disk_iops += n_info.disk_iops;
   	total_disk_capacity += n_info.disk_capacity;
   	disk_latency = n_info.disk_latency;
   	total_ssd_iops += n_info.ssd_iops;
   	total_ssd_capacity += n_info.ssd_capacity;
   	ssd_latency = n_info.ssd_latency;

     
     FDS_PLOG(parent_log) << "Total Disk Resources "
                     << "  Total Disk iops : " << total_disk_iops
                     << "  Total Disk capacity : " << total_disk_capacity
                     << "  Disk latency : " << disk_latency
                     << "  Total Ssd iops : " << total_ssd_iops
                     << "  Total Ssd capacity : " << total_ssd_capacity
                     << "  Ssd latency : " << ssd_latency;

   }

   void FdsAdminCtrl::getAvailableDiskCapacity(class VolumeInfo  *pVolInfo ) {  

   }


   void FdsAdminCtrl::updatetAvailableDiskCapacity(class VolumeInfo  *pVolInfo ) {  
   }

   void FdsAdminCtrl::initDiskCapabilities() {

   	total_disk_iops = 0;
   	total_disk_capacity = 0;
   	disk_latency = 0;
   	total_ssd_iops = 0;
   	total_ssd_capacity = 0;
   	ssd_latency = 0;

   /* Available  capacity */
   	avail_disk_iops = 0;
   	avail_disk_capacity = 0;
   	avail_ssd_iops = 0;
   	avail_ssd_capacity = 0;
   }
}
