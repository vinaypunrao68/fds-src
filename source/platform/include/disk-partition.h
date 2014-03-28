/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_PLATFORM_INCLUDE_DISK_PARTITION_H_
#define SOURCE_PLATFORM_INCLUDE_DISK_PARTITION_H_

#include <vector>
#include <disk.h>
#include <parted/parted.h>

namespace fds {
class DiskPartition : public DiskObjIter
{
  public:
    explicit DiskPartition(DiskObj::pointer disk);
    virtual ~DiskPartition();

    virtual bool dsk_iter_fn(DiskObj::pointer curr);

  protected:
    DiskObj::pointer          pe_disk;
    PedDevice                *pe_dev;
};

}  // namespace fds
#endif  // SOURCE_PLATFORM_INCLUDE_DISK_PARTITION_H_
