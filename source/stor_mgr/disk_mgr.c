#include <fds_commons.h>
#include <stor_mgr.h>
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

typedef struct {
  fds_uint64_t      max_capacity;
  fds_uint32_t      used_capacity;
  fds_uint32_t      num_sectors;
  fds_disk_type_t   disk_type;           /* SSD or SATA/SAS */
  fds_char_t        disk_dev_name[64];  /* This is the /dev/sda1  during discovery */
  fds_char_t        xfs_mount_point_name[128];
  fds_disk_status_t disk_status;
  fds_disk_stats_t    disk_stats;
} fds_disk_info_t;

fds_disk_info_t  disk_info[FDS_MAX_DISKS_SYSTEM+1];

void fds_disk_mgr_init() {
int i=0;

  for(i =1; i <= FDS_MAX_DISKS_SYSTEM; i++) {
   disk_info[i].max_capacity = 2048*1024*1024; // capacity in KB
   disk_info[i].used_capacity = 0;
   disk_info[i].num_sectors = disk_info[i].max_capacity*2;
   disk_info[i].disk_type = FDS_DISK_SATA;
   sprintf(disk_info[i].disk_dev_name, "/dev/sda%d", i);
   sprintf(disk_info[i].xfs_mount_point_name, "/fds/mnt/disk%d", i);
   disk_info[i].disk_status = FDS_DISK_UP;
  }
}


void fds_disk_mgr_main() {
  fds_disk_mgr_init();

}


void fds_disk_mgr_exit() {

}
