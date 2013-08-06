#include "include/fds_types.h"
#include <StorMgr.h>
#include <DiskMgr.h>
DiskInfoType  disk_info[FDS_MAX_DISKS_SYSTEM+1];


DiskMgr::DiskMgr() {
  int i=0;
  fds_char_t shard_file_name[64];
  
  for(i =1; i <= FDS_MAX_DISKS_SYSTEM; i++) {
    disk_info[i].max_capacity = (fds_uint64_t)2048*1024*1024; // capacity in KB
    disk_info[i].used_capacity = 0;
    disk_info[i].num_sectors = disk_info[i].max_capacity*2;
    disk_info[i].disk_type = FDS_DISK_SATA;
    sprintf(disk_info[i].disk_dev_name, "/dev/sda%d", i);
    sprintf(disk_info[i].xfs_mount_point_name, "/fds/mnt/disk%d", i);
    disk_info[i].disk_status = FDS_DISK_UP;
    disk_info[i].shard_file_desc = NULL;
    disk_info[i].shard_file_num = 1;
    memset(shard_file_name, 0x0, 64);
    sprintf(shard_file_name, "%s/shard_file.%d", disk_info[i].xfs_mount_point_name, disk_info[i].shard_file_num);
    disk_info[i].shard_file_desc = fopen(shard_file_name, "w+");
    if (disk_info[i].shard_file_desc != NULL) {
      fseek(disk_info[i].shard_file_desc, 0L, SEEK_END);
    }
  }
}


DiskMgr::~DiskMgr() {
int i=0;
// fds_char_t shard_file_name[64];

  for(i =1; i <= FDS_MAX_DISKS_SYSTEM; i++) {
    fclose(disk_info[i].shard_file_desc);
  }
}


fds_sm_err_t DiskMgr::writeObject(FDS_ObjectIdType *object_id, 
                                   fds_uint32_t obj_len, 
                                   fds_char_t *object,
                                   FDSDataLocEntryType *data_loc,
				   fds_uint32_t disk_num)
{
fds_shard_obj_hdr_t shard_hdr;
   memset(&shard_hdr, 0 , sizeof(fds_shard_obj_hdr_t));
   data_loc->shard_file_offset = ftell(disk_info[disk_num].shard_file_desc);
   data_loc->disk_num = disk_num;
   //strncpy(data_loc->shard_file_name, disk_info[disk_num].shard_file_name, 64);
   memcpy(&shard_hdr.data_obj_id, object_id, sizeof(FDS_ObjectIdType));
   shard_hdr.data_obj_len = obj_len;
   fwrite(&shard_hdr, sizeof(fds_shard_obj_hdr_t), 1, disk_info[disk_num].shard_file_desc);
   fwrite(&object, shard_hdr.data_obj_len, 1, disk_info[disk_num].shard_file_desc);

   return FDS_SM_OK;
}
