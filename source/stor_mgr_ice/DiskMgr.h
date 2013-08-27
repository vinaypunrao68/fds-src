#ifndef __DISK_MGR_H__
#define __DISK_MGR_H__
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "FDSP.h"
#include "stor_mgr_err.h"
#include "ObjLoc.h"
#include "odb.h"
#include <unistd.h>
#include <assert.h>
#include "odb.h"
#include <iostream>
#include <Ice/Ice.h>
#include <DiskMgr.h>


using namespace FDS_ProtocolInterface;

#define FDS_MAX_DISKS_SYSTEM 26
typedef enum {
FDS_DISK_EMPTY,
FDS_DISK_UP,
FDS_DISK_EXPUNGED,
FDS_DISK_DOWN
} fds_disk_status_t;


typedef enum {
  FDS_DISK_SSD,
  FDS_DISK_SATA
} fds_disk_type_t;

typedef struct {
 fds_uint32_t   disk_io_errors;
 fds_uint32_t   disk_rd_timeout;
 fds_uint32_t   disk_wr_timeout;
} fds_disk_stats_t;


class DiskInfoType {
public: 
  fds_uint64_t      max_capacity;
  fds_uint32_t      used_capacity;
  fds_uint64_t      num_sectors;
  fds_disk_type_t   disk_type;           /* SSD or SATA/SAS */
  fds_char_t        disk_dev_name[64];  /* This is the /dev/sda1  during discovery */
  fds_char_t        xfs_mount_point_name[128];
  fds_disk_status_t disk_status;
  fds_disk_stats_t  disk_stats;
  FILE              *shard_file_desc;
  fds_uint32_t      shard_file_num; // Present shard file number being filled up
  fds_char_t shard_file_name[64];
};

typedef struct {
  ObjectID  data_obj_id;
  fds_uint32_t     data_obj_len;
} fds_shard_obj_hdr_t;

class DiskMgr {
public :
   DiskInfoType diskInfo[FDS_MAX_DISKS_SYSTEM];

   DiskMgr();
   ~DiskMgr();

   fds_sm_err_t writeObject(FDS_ObjectIdType     *object_id, 
		            fds_uint32_t         obj_len, 
		            fds_char_t          *object,
		            FDSDataLocEntryType *data_loc,
		            fds_uint32_t         disk_num);
};


#endif
