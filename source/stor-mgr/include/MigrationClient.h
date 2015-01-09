/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_MIGRATIONCLIENT_H_
#define SOURCE_STOR_MGR_INCLUDE_MIGRATIONCLIENT_H_


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
                             NodeUuid& _destinationSMNodeID,
                             fds_token_id _sm_tokenID);
    ~MigrationClient();

    typdef std::unique_ptr<MigrationClient> unique_ptr;

    /**
     * A simple routine to snapshot metadata associated with the token.
     */
    Error snapshotObjects();


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


  private:
    /**
     * SM token to filter and send back
     *
     * TODO(Sean): Does this have to be DLT or SM token?
     */
    fds_token_id SMTokenID;

    /**
     * destination SM node ID.  This is the SM Node ID that's requesting the
     * the set of objects associated with the
     */
    NodeUuid destinationSMNodeID;

    /**
     * Object data store handler.  Set during the initialization.
     */
    SmIoReqHandler *dataStore;

    /**
     * Request to snapshot metadata DB for the assigned token.
     */
    SmIoSnapshotObjectDB snapshotRequest;

    /**
     * The list of the objects from the destination SM.  This list is filtered
     * against the source SM to determine which objects in the token file should
     * be migrated from the source SM to destination SM.
     *
     * TODO(Sean):  This is just a place holder to know that we need to keep
     *              a list (or set) of object to filter against the source SM's
     *              token snapshot.
     */
    std::list filterObjectsList;

    /**
     * Pointer to the snapshot.  This is set in the snapshot callback.
     */
    leveldb::DB *snapshotLevelDB;

    /**
     * Pointer to the snapshot iterator.  We need to save the iterator,
     * since we need to keep the
     */
    leveldb::Iterator *iteratorLevelDB;
};  // class MigrationClient

}  // namespace fds



#endif  // SOURCE_STOR_MGR_INCLUDE_MIGRATIONCLIENT_H_
