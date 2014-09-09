/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_STOR_MGR_ERR_H_
#define SOURCE_STOR_MGR_INCLUDE_STOR_MGR_ERR_H_

// This is the internal Storage manager errors for functions that pass each other within SM
typedef enum {
    FDS_SM_OK,
    FDS_SM_ERR_DUPLICATE,
    FDS_SM_ERR_DISK_WRITE_FAILED,
    FDS_SM_ERR_DISK_READ_FAILED,
    FDS_SM_ERR_OBJ_NOT_EXIST,
    FDS_SM_ERR_HASH_COLLISION
} fds_sm_err_t;

#endif  // SOURCE_STOR_MGR_INCLUDE_STOR_MGR_ERR_H_
