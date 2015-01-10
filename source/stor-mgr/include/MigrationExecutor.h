/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_MIGRATIONEXECUTOR_H_
#define SOURCE_STOR_MGR_INCLUDE_MIGRATIONEXECUTOR_H_

#include <memory>
#include <mutex>
#include <set>

#include <fds_types.h>
#include <SmIo.h>

namespace fds {

class MigrationExecutor {
  public:
    MigrationExecutor(SmIoReqHandler *_dataStore,
                      fds_uint32_t bitsPerToken,
                      const NodeUuid& srcSmId,
                      fds_token_id smTokId);
    ~MigrationExecutor();

    typedef std::unique_ptr<MigrationExecutor> unique_ptr;

    /**
     * Adds DLT token to the list of DLT tokens for which this
     * MigrationExecutor is responsible for
     */
    void addDltToken(fds_token_id dltTok);

    /**
     * Start the object rebalance.  The rebalance inintiated by the
     * destination SM.
     */
    Error startObjectRebalance(leveldb::ReadOptions& options,
                               leveldb::DB *db);

  private:
    /**
     * Object data store handler.  Set during the initialization.
     */
    SmIoReqHandler *dataStore;

    /**
     * Source SM id
     */
    NodeUuid sourceSmUuid;

    /**
     * SM Token to migration from the source SM node.
     */
    fds_token_id smTokenId;

    /**
     * Set of DLT tokens that needs to be migrated from source SM
     * SM token contains one or more DLT tokens
     */
    std::set<fds_token_id> dltTokens;
    fds_uint32_t bitsPerDltToken;

    /// true if standalone (no rpc sent)
    fds_bool_t testMode;
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_MIGRATIONEXECUTOR_H_
