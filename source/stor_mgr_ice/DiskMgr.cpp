#include "include/fds_types.h"
#include <StorMgr.h>
#include <DiskMgr.h>


DiskMgr::DiskMgr() {
  int i=0;
  
  for(i =1; i < FDS_MAX_DISKS_SYSTEM; i++) {
    diskInfo[i].max_capacity = (fds_uint64_t)2048*1024*1024; // capacity in KB
    diskInfo[i].used_capacity = 0;
    diskInfo[i].num_sectors = diskInfo[i].max_capacity*2;
    diskInfo[i].disk_type = FDS_DISK_SATA;
    sprintf(diskInfo[i].disk_dev_name, "/dev/sda%d", i);
    sprintf(diskInfo[i].xfs_mount_point_name, "/fds/mnt/disk%d", i);
    diskInfo[i].disk_status = FDS_DISK_UP;
    diskInfo[i].shard_file_desc = NULL;
    diskInfo[i].shard_file_num = 1;
    memset(diskInfo[i].shard_file_name, 0x0, 64);
    sprintf(diskInfo[i].shard_file_name, "%s/shard_file.%d", diskInfo[i].xfs_mount_point_name, diskInfo[i].shard_file_num);
    diskInfo[i].shard_file_desc = fopen(diskInfo[i].shard_file_name, "w+");
    if (diskInfo[i].shard_file_desc != NULL) {
      fseek(diskInfo[i].shard_file_desc, 0L, SEEK_END);
    }
  }
}


DiskMgr::~DiskMgr() {
 int i=0;

  for(i =1; i < FDS_MAX_DISKS_SYSTEM; i++) {
    if (diskInfo[i].shard_file_desc)
	    fclose(diskInfo[i].shard_file_desc);
  }
}


fds_sm_err_t DiskMgr::writeObject(FDS_ObjectIdType *object_id, 
                                   fds_uint32_t obj_len, 
                                   fds_char_t *object,
                                   FDS_DataLocEntry *data_loc,
				   fds_uint32_t disk_num)
{
fds_shard_obj_hdr_t shard_hdr;
   memset(&shard_hdr, 0 , sizeof(fds_shard_obj_hdr_t));
   data_loc->shard_file_offset = ftell(diskInfo[disk_num].shard_file_desc);
   data_loc->disk_num = disk_num;
   strncpy(data_loc->shard_file_name, diskInfo[disk_num].shard_file_name, 64);
   memcpy(&shard_hdr.data_obj_id, object_id, sizeof(FDS_ObjectIdType));
   shard_hdr.data_obj_len = obj_len;
   fwrite(&shard_hdr, sizeof(fds_shard_obj_hdr_t), 1, diskInfo[disk_num].shard_file_desc);
   fwrite(&object, shard_hdr.data_obj_len, 1, diskInfo[disk_num].shard_file_desc);

   return FDS_SM_OK;
}
