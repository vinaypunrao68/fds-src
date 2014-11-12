/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_CHECKER_SMCHK_DRIVER_H_
#define SOURCE_CHECKER_SMCHK_DRIVER_H_

#include "smchk.h"

namespace fds {
class SMChkDriver : public FdsProcess {
  public:
    SMChkDriver(int argc, char * argv[], const std::string &config,
            const std::string &basePath, Module * vec[],
            fds::SmDiskMap::ptr smDiskMap,
            fds::ObjectDataStore::ptr smObjStore);
    ~SMChkDriver() {}

    int run() override;

  private:
    SMChk *checker;
    RunFunc cmd;
    fds::SmDiskMap::ptr smDiskMap;
    fds::ObjectDataStore::ptr smObjStore;
    fds::ObjectMetadataDb::ptr smMdDb;
    int sm_count;
};
}  // namespace fds

#endif