/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_TOOLS_SHM_DUMP_H_
#define SOURCE_TOOLS_SHM_DUMP_H_

#include <string>

namespace fds {
class ShmDump {
  public:
    static void read_inv_shm(const char *fname);
    static void read_queue_shm(const char *fname);

  private:
    ShmDump() {}
    ~ShmDump() {}
};

}  // namespace fds

#endif  // SOURCE_TOOLS_SHM_DUMP_H_
