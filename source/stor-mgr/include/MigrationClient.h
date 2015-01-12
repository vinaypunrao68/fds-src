/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_MIGRATIONCLIENT_H_
#define SOURCE_STOR_MGR_INCLUDE_MIGRATIONCLIENT_H_

#include <list>
#include <map>

#include <fds_types.h>
#include <SmIo.h>

namespace fds {

/**
 * This is the client class for token migration.  This class is instantiated by the
 * source SM.
 * It will receive the set of object information from the destination SM, and this
 * set of objects is filtered against the local snapshot of the metadata to determine
 * the delta set of objects between the source and destination SM.
 * The objects in the delta set is pushed from this SM node to the destination
 * SM node that has requested the token migration.
 */
class MigrationClient {
  public:
    explicit MigrationClient(SmIoReqHandler *_dataStore,
                             NodeUuid& _destinationSMNodeID);
    ~MigrationClient();

    typedef std::unique_ptr<MigrationClient> unique_ptr;
    typedef std::shared_ptr<MigrationClient> shared_ptr;

    /**
     * A simple routine to snapshot metadata associated with the token.
     */
    Error migClientSnapshotMetaData();

    /**
     * Callback from leveldb snapshot.
     *
     * TODO(Sean): Not sure how this is going to work out.  This is going to
     *             filter against the destination SM objects.
     *             The threading mode + state machine needs further thinking.
     *             Since, not sure what this callback is going to do, making
     *             it a generic name.
     */
    void migClientSnapshotCB(const Error& error,
                             SmIoSnapshotObjectDB* snapRequest,
                             leveldb::ReadOptions& options,
                             leveldb::DB *db);

    /**
     * Add set of dlt tokens to the client.
     */
    void migClientAddDltTokens(fds_token_id dltToken);

    /**
     * Add set of Object IDs and refcnt to the client map
     */
    void migClientAddDestSet(fpi::CtrlObjectRebalanceInitialSetPtr &initialSet);

    /**
     * Set sequence number
     */
    void migClientSetSeqNum(uint64_t seqNum);

  private:
    /**
     * SM token which is derived from the set of DLT tokens.
     */
    fds_token_id SMTokenID;

    /**
     * Set of dlt tokens to filter against the the snapshot.
     */
    std::unordered_set<fds_token_id> dltTokenIDs;

    /**
     * The list of the objects from the destination SM.  This list is filtered
     * against the source SM to determine which objects in the token file should
     * be migrated from the source SM to destination SM.
     *
     * TODO(Sean):  This is just a place holder to know that we need to keep
     *              a list (or set) of object to filter against the source SM's
     *              token snapshot.
     */
    std::map<ObjectID, uint64_t> filterObjectsList;

    /**
     * Current sequence number.  The number is increased sequently to determine
     * that all incremental messages are received by the migration client from
     * the migration executor.
     *
     * TODO(Sean):  This should be a utility class that maintains the sequernce
     *              number.
     */
     uint64_t seqNum;

    /**
     * destination SM node ID.  This is the SM Node ID that's requesting the
     * the set of objects associated with the
     */
    NodeUuid destSMNodeID;

    /**
     * Object data store handler.  Set during the initialization.
     */
    SmIoReqHandler *dataStore;

    /**
     * Request to snapshot metadata DB for the assigned token.
     */
    SmIoSnapshotObjectDB snapshotRequest;

    /**
     * Pointer to the snapshot.  This is set in the snapshot callback.
     */
    leveldb::DB *snapshotLevelDB;

    /**
     * Pointer to the snapshot iterator.  We need to save the iterator,
     * since we need to keep the pointer to the current iteration.
     */
    leveldb::Iterator *iteratorLevelDB;
};  // class MigrationClient

}  // namespace fds



#endif  // SOURCE_STOR_MGR_INCLUDE_MIGRATIONCLIENT_H_
