/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_PLATFORM_FDS_SHMEM_H_
#define SOURCE_INCLUDE_PLATFORM_FDS_SHMEM_H_

#include <stdlib.h>

namespace fds {

class FdsShmem
{
  public:
    FdsShmem();
    virtual ~FdsShmem();

  protected:
    const char               *sh_name;
    void                     *sh_addr;
    size_t                    sh_size;
};

}  // namespace fds
#endif  // SOURCE_INCLUDE_PLATFORM_FDS_SHMEM_H_
