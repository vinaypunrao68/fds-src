/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONMGR_H_
#define SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONMGR_H_

#include <fds_error.h>

namespace fds {

// Forward declaration
class DmIoReqHandler;

// callback for start migration ack back to OM.
typedef std::function<void (const Error&)> OmStartMigrationCBType;

class DmMigrationMgr {
  public:
    explicit DmMigrationMgr(DmIoReqHandler* DmReqHandle);
    ~DmMigrationMgr();

    typedef std::unique_ptr<DmMigrationMgr> unique_ptr;
    typedef std::shared_ptr<DmMigrationMgr> shared_ptr;

  private:
    DmIoReqHandler* DmReqHandler;

    // Ack back to DM start migration from the Destination DM to OM.
    OmStartMigrationCBType OmStartMigrCb;

};  // DmMigrationMgr

}  // namespace fds


#endif  // SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONMGR_H_
