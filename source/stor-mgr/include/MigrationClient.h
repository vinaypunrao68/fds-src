/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_MIGRATIONCLIENT_H_
#define SOURCE_STOR_MGR_INCLUDE_MIGRATIONCLIENT_H_

#include <list>
#include <map>

#include <fds_types.h>

#include <SmIo.h>

#include <MigrationUtility.h>

namespace fds {

const fds_token_id SMTokenInvalidID = 0xffffffff;
const uint64_t invalidExecutorID = 0xffffffffffffffff;

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
                             fds_uint32_t bitsPerToken);
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
     * Add initial set of DLT and Objects to the clients
     * The thrift message contains the DLT, objects associated with the DLT,
     * sequence number and a flag to indicate the last message.
     *
     * return if all the set of objects (sequence numbers are complete
     * and no holes) are added.
     *
     * @return true - all sequence numbers are received
     *
     * TODO(Sean):  For fault tolerance, we need to setup a timer routine to
     *              check if the sequence number is moving.  So, if the
     *              sequence number isn't updated for a long time, take the
     *              node offline.
     */
    Error migClientAddObjToFilterSet(fpi::CtrlObjectRebalanceFilterSetPtr& filterSet);

    /**
     * Callback from the QoS
     */
    void
    migClientReadObjDeltaSetCb(const Error& error,
                               SmIoReadObjDeltaSetReq *req);

  private:
    /* Verify that set of DLT tokens belong to the same SM token.
     */
    bool migClientVerifyDestination(fds_token_id dltToken,
                                    fds_uint64_t executorId);

    /* Add object meta data to the set to be sent to QoS.
     */
    void migClientAddMetaData(std::vector<ObjMetaData::ptr>& objMetaDataSet,
                              fds_bool_t lastSet);

    /**
     * Return sequence number for delta set message from source SM to
     * destination SM.
     */
    fds_uint64_t
    getSeqNumDeltaSet() {
        return seqNumDeltaSet++;
    };

    /**
     * Reset sequence number
     */
    void
    resetSeqNumDeltaSet() {
        seqNumDeltaSet = 0UL;
    };


    /**
     * SM token which is derived from the set of DLT tokens.
     */
    fds_token_id SMTokenID;

    /**
     * bits per dlt token.
     */
    fds_uint32_t bitsPerDltToken;

    /**
     * Mutex for dltTokenIDs and filterObjectSet.
     */
    std::mutex filterSetLock;

    /**
     * Set of dlt tokens to filter against the the snapshot.
     */
    std::unordered_set<fds_token_id> dltTokenIDs;

    /**
     * The list of the objects from the destination SM.  This list is filtered
     * against the source SM to determine which objects in the token file should
     * be migrated from the source SM to destination SM.
     *
     * <objectID + refCnt>
     *
     * TODO(Sean):  Need to optimize this.  If sizing is issue, we may
     *              need multiple set of object list and filter against
     *              snapshot multiple times.
     *              1MB can hold 37,449 objects (28 bytes for <objectID+refcnt>)
     *              1GB can hold 38,347,776 objects...
     *
     * TODO(Sean): Second optimization is how to filter this object lists against the
     *             source SM snapshot.  If filterObjectList is big, then we have to something
     *             to minimize the memory signature.  One way to do it sort both
     *             filterObjectList and snapshot, and iterate like merge sort to get
     *             unique objects.
     */
    std::unordered_map<ObjectID, fds_uint64_t, ObjectHash> filterObjectSet;

    /**
     * Maintain the message from the destination SM to determine if all
     * sequences are received by the client.  Since the message from the
     * destination SM is asynchronous, the client can receive them
     * out of order.  This is to ensure that all messages are received.
     */
    MigrationSeqNum seqNumFilterSet;

    /**
     * destination SM node ID.  This is the SM Node ID that's requesting the
     * the set of objects associated with the
     */
    NodeUuid destSMNodeID;

    /**
     * Id of migrationExecutor ID.  This is unique to the destination SM.
     * With destination SM UUID + executor ID, we can find the
     * corresponding executor instance on the destination SM.
     */
    fds_uint64_t executorID;

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
    leveldb::DB *snapDB;

    /**
     * Pointer to the snapshot iterator.  We need to save the iterator,
     * since we need to keep the pointer to the current iteration.
     */
    leveldb::Iterator *iterDB;

    /**
     * Maximum number of objects to send in delta set back to the destination SM.
     */
    fds_uint32_t maxDeltaSetSize;

    /**
     * Maintain the sequence number for the delta set of object to be sent
     * from the source SM to destination SM.
     */
    fds_uint64_t seqNumDeltaSet;

    /**
     * Standalone test mode.
     */
    fds_bool_t testMode;
};  // class MigrationClient

}  // namespace fds



#endif  // SOURCE_STOR_MGR_INCLUDE_MIGRATIONCLIENT_H_
