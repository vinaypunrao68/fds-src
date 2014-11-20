/*
 * Copyright 2014 by Formation Data Sysems, Inc.
 */
#ifndef SOURCE_INCLUDE_SHARED_FDS_CONSTANTS_H_
#define SOURCE_INCLUDE_SHARED_FDS_CONSTANTS_H_

#include <shared/fds_types.h>

/*  This file is in the shared directory.  Keep it in C code. */
c_decls_begin

/**
 * -----------------------------------------------------------------------------------
 * Some max constants.
 * -----------------------------------------------------------------------------------
 */
#define FDS_MAX_FILE_NAME              (256)
#define FDS_MAX_IP_STR                 (32)
#define FDS_MAX_UUID_STR               (32)
#define FDS_MAX_NODE_NAME              (12)
#define MAX_SVC_NAME_LEN               (12)
#define FDS_MAX_VOL_NAME               (32)
#define MAX_DOMAIN_EP_SVC              (10240)

/**
 * -----------------------------------------------------------------------------------
 * System platform constants.
 * -----------------------------------------------------------------------------------
 */
#define FDS_MAX_CPUS                   (32)
#define FDS_MAX_DISKS_PER_NODE         (1024)
#define FDS_MAX_AM_NODES               (10000)
#define MAX_DOMAIN_NODES               (1024)

#define FDS_MAX_DLT_ENTRIES            (65536)
#define FDS_MAX_DLT_DEPTH              (4)
#define FDS_MAX_DLT_BYTES              (522 * 1024)
// current default DMT size
#define FDS_MAX_DMT_BYTES              (16 * 4 * 8 + 20)

c_decls_end

#endif  /* SOURCE_INCLUDE_SHARED_FDS_CONSTANTS_H_  // NOLINT */
