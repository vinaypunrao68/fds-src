#ifndef __STOR_MGR_ERR_H__
#define __STOR_MGR_ERR_H__
// This is the internal Storage manager errors for functions that pass each other within SM
typedef enum {
FDS_SM_OK,
FDS_SM_ERR_DUPLICATE,
FDS_SM_ERR_DISK_WRITE_FAILED,
FDS_SM_ERR_DISK_READ_FAILED,
} fds_sm_err_t;

#endif
