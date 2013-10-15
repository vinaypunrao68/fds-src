/*
 * Addmission Control class 
 *   formation Data systems 
 */
#ifndef ORCH_MGR_ADMINCTRL_H_
#define ORCH_MGR_ADMINCTRL_H_

#include <Ice/Ice.h>
#include <unordered_map>
#include <string>

#include <fds_volume.h>
#include <fdsp/FDSP.h>
#include <util/Log.h>
#include "orchMgr.h"

namespace fds {

class FdsAdminCtrl {

public:
  FdsAdminCtrl(const std::string& om_prefix, fds_log* om_log);
  ~FdsAdminCtrl();
  /* Per local domain  dynamic disk resource  counters */
   fds_uint32_t  total_disk_iops_max;
   fds_uint32_t  total_disk_iops_min;
   double  total_disk_capacity;
   fds_uint32_t  disk_latency_max;
   fds_uint32_t  disk_latency_min;
   fds_uint32_t  total_ssd_iops_max;
   fds_uint32_t  total_ssd_iops_min;
   double  total_ssd_capacity;
   fds_uint32_t  ssd_latency_max;
   fds_uint32_t  ssd_latency_min;

   /* Available  capacity */
   fds_uint32_t  avail_disk_iops_max;
   fds_uint32_t  avail_disk_iops_min;
   double        avail_disk_capacity;
   fds_uint32_t  avail_ssd_iops_max;
   fds_uint32_t  avail_ssd_iops_min;
   double        avail_ssd_capacity;

   void addDiskCapacity( class NodeInfo& n_info);
   void getAvailableDiskCapacity(class VolumeInfo  *pVolInfo);  
   void updatetAvailableDiskCapacity(class VolumeInfo  *pVolInfo);
   void volAdminControl(class VolumeInfo  *pVolInfo);
   void initDiskCapabilities();

  /* parent log */
  fds_log* parent_log;
}; 

}
#endif
