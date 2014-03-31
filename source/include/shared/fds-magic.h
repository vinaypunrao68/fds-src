/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_SHARED_FDS_MAGIC_H_
#define SOURCE_INCLUDE_SHARED_FDS_MAGIC_H_

/*
 * This file is shared, keep it in C code.
 */
#include <shared/fds_types.h>

c_decls_begin

#define MAGIC_DSK_SUPPER_BLOCK              (0x46445344)   /*  FDSD in ascii */
#define MAGIC_DSK_UUID_REC                  (0x4449534B)   /*  DISK in ascii */

c_decls_end

#endif  /* SOURCE_INCLUDE_SHARED_FDS_MAGIC_H_  // NOLINT */
