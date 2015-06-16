/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONCLIENT_H_
#define SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONCLIENT_H_

namespace fds {

// Forward declaration.
class DmIoReqHandler;

class DmMigrationClient {
  public:
    explicit DmMigrationClient(DmIoReqHandler* DmReqHandle);
    ~DmMigrationClient();

    typedef std::unique_ptr<DmMigrationClient> unique_ptr;
    typedef std::shared_ptr<DmMigrationClient> shared_ptr;

  private:
    DmIoReqHandler* DmReqHandler;

};  // DmMigrationClient


}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONCLIENT_H_

