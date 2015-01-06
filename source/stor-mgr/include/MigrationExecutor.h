/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_MIGRATIONEXECUTOR_H_
#define SOURCE_STOR_MGR_INCLUDE_MIGRATIONEXECUTOR_H_

#include <memory>
#include <mutex>
#include <condition_variable>

#include <fds_types.h>

#include <persistent-layer/dm_io.h>

#include <ObjMeta.h>
#include <SmIo.h>

namespace fds {

class MigrationExecutor {
  public:
    explicit MigrationExecutor(SmIoReqHandler *_dataStore,
                               uint64_t _sourceNodeID,
                               fds_token_id _smTokenId);
    ~MigrationExecutor();

    typedef std::unique_ptr<MigrationExecutor> unique_ptr;

    /**
     * Start the object rebalance.  The rebalance inintiated by the
     * destination SM.
     */
    Error startObjectRebalance();

  private:
    /**
     * Callback function from the metadata snapshot request.  After
     * the snapshot, generate the set of objects to rebalanc/sync.
     */
    void getObjectRebalanceSet(const Error& error,
                               SmIoSnapshotObjectDB* snapRequest,
                               leveldb::ReadOptions& options,
                               leveldb::DB *db);
    /**
     * Object data store handler.  Set during the initialization.
     */
    SmIoReqHandler *dataStore;

    /**
     * Request to snapshot metadata DB for the assigned token.
     */
    SmIoSnapshotObjectDB snapshotRequest;

    /**
     * Source SM id
     */
    NodeUuid sourceSMNodeID;

    /**
     * Token to migration from the source SM node.
     */
    fds_token_id tokenID;

    /**
     * metadata snapshot is done by another thread's context, so requires
     * a set of synchronization to wait until the snapshot is complete.
     */
    std::mutex metadataSnapshotMutex;
    std::condition_variable metadataSnapshotCondVar;
    bool metadataSnapshotCompleted;
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_MIGRATIONEXECUTOR_H_
