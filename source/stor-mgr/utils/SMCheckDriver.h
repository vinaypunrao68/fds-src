/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_UTILS_SMCHECK_DRIVER_H_
#define SOURCE_STOR_MGR_UTILS_SMCHECK_DRIVER_H_

#include <SMCheck.h>

namespace fds {
class SMCheckDriver : public FdsProcess {
  public:
    SMCheckDriver(int argc, char * argv[], const std::string &config,
                  const std::string &basePath, Module * vec[],
                  SmDiskMap::ptr smDiskMap,
                  ObjectDataStore::ptr smObjStore);
    ~SMCheckDriver() {}

    int run() override;

  private:
    SMCheck *checker;
    RunFunc cmd;
    SmDiskMap::ptr smDiskMap;
    ObjectDataStore::ptr smObjStore;
    ObjectMetadataDb::ptr smMdDb;
    bool verbose;
    bool ownershipCheck;
    bool checkOnlyActive;
};
}  // namespace fds

#endif  // SOURCE_STOR_MGR_UTILS_SMCHK_DRIVER_H_
