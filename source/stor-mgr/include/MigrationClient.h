/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_MIGRATIONCLIENT_H_
#define SOURCE_STOR_MGR_INCLUDE_MIGRATIONCLIENT_H_

#include <list>
#include <map>
#include <atomic>
#include <condition_variable>

#include <fds_types.h>

#include <SmIo.h>
#include <odb.h>
#include <MigrationUtility.h>
#include <MigrationTools.h>

namespace fds {

/* Forward declarations */
struct EPSvcRequest;

const fds_token_id SMTokenInvalidID = 0xffffffff;
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
                             fds_uint64_t& targetDltVersion,
                             fds_uint32_t bitsPerToken,
                             bool onePhaseMigration);
    ~MigrationClient();

     enum MigrationClientState {
            MC_INIT,
            MC_FILTER_SET,
            MC_FIRST_PHASE_DELTA_SET,
            MC_FIRST_PHASE_DELTA_SET_COMPLETE,
            MC_SECOND_PHASE_DELTA_SET,
            MC_SECOND_PHASE_DELTA_SET_COMPLETE,
            MC_DONE,
            MC_ERROR
     };

    typedef std::unique_ptr<MigrationClient> unique_ptr;
    typedef std::shared_ptr<MigrationClient> shared_ptr;

    typedef std::function<void()> continueWorkFn;

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
    void migClientSnapshotFirstPhaseCb(const Error& error,
                                       SmIoSnapshotObjectDB* snapRequest,
                                       std::string &snapDir,
                                       leveldb::CopyEnv *env);

    void migClientSnapshotSecondPhaseCb(const Error& error,
                                        SmIoSnapshotObjectDB* snapRequest,
                                        std::string &snapDir,
                                        leveldb::CopyEnv *env);

    /**
     * Add initial set of DLT and Objects to the clients
     * The thrift message contains the DLT, objects associated with the DLT,
     * sequence number and a flag to indicate the last message.
     *
     * return if all the set of objects (sequence numbers are complete
     * and no holes) are added.
     *
     * @param[in] srcAccepted true if this SM is accepted to be a source for
     *            DLT token in given filterSet. Otherwise false and this method
     *            will just check if all set of objects are received (including
     *            DLT tokens that were accepted) and if so, will proceed to next
     *            state.
     * @return true - all sequence numbers are received
     *
     * TODO(Sean):  For fault tolerance, we need to setup a timer routine to
     *              check if the sequence number is moving.  So, if the
     *              sequence number isn't updated for a long time, take the
     *              node offline.
     */
    Error migClientStartRebalanceFirstPhase(fpi::CtrlObjectRebalanceFilterSetPtr& filterSet,
                                            fds_bool_t srcAccepted);

    Error migClientStartRebalanceSecondPhase(fpi::CtrlGetSecondRebalanceDeltaSetPtr& secondPhaseMsg);

    /**
     * Callback from the QoS
     */
    void migClientReadObjDeltaSetCb(const Error& error,
                                    SmIoReadObjDeltaSetReq *req,
                                    continueWorkFn nextWork);

    /**
     * Will set forwarding flag to true
     * @param smToken is SM token, for sanity check
     */
    void setForwardingFlagIfSecondPhase(fds_token_id smTok);

    /**
     * Forwards requests if given DLT token is migrating and migration
     * client has forwarding flag set
     * @return true if request was forwarded
     */
    fds_bool_t forwardIfNeeded(fds_token_id dltToken,
                               FDS_IOType* req);
    fds_bool_t forwardAddObjRefIfNeeded(fds_token_id dltToken,
                                        fpi::AddObjectRefMsgPtr addObjRefReq);

    /**
     * Wait for all pending Client requests to complete.
     */
    void waitForIOReqsCompletion(fds_uint64_t executorId);

  private:
    /*
     * Builds delta sets for first phase until a levelDB iterator is exhausted.
     * Takes a levelDB iterator pointer as a parameter. If this is nullptr we'll assume we were in error and delete
     * the iterator.
     */
    void buildDeltaSetWorkerFirstPhase(leveldb::Iterator *iterDB,
                                       leveldb::DB *db,
                                       std::string &firstPhaseSnapDir,
                                       leveldb::CopyEnv *env);

    /*
     * Builds delta sets for second phase until a levelDB iterator is exhausted.
     * Takes a levelDB iterator pointer as a parameter. If this is nullptr we'll assume we were in error and delete
     * the iterator.
     */
    void buildDeltaSetWorkerSecondPhase(metadata::metadata_diff_type::iterator start,
                                        metadata::metadata_diff_type::iterator end);

    // Cleans up some things after the second phase is complete
    void migrationCleanup(leveldb::CopyEnv *env,
                          std::string firstPhaseSnapDir,
                          std::string secondPhaseSnapDir);

    /* Verify that set of DLT tokens belong to the same SM token.
     */
    bool migClientVerifyDestination(fds_token_id dltToken,
                                    fds_uint64_t executorId);

    /* Add object meta data to the set to be sent to QoS.
     */
    void migClientAddMetaData(std::vector<std::pair<ObjMetaData::ptr,
                                                    fpi::ObjectMetaDataReconcileFlags>>& objMetaDataSet,
                              fds_bool_t lastSet, continueWorkFn nextWork);

    void fwdPutObjectCb(SmIoPutObjectReq* putReq,
                        EPSvcRequest* svcReq,
                        const Error& error,
                        boost::shared_ptr<std::string> payload);

    void fwdDelObjectCb(SmIoDeleteObjectReq* delReq,
                        EPSvcRequest* svcReq,
                        const Error& error,
                        boost::shared_ptr<std::string> payload);

    /**
     * In error, sets error state and report error to OM
     */
    void handleMigrationError(const Error& error);

    /**
     * Return sequence number for delta set message from source SM to
     * destination SM.
     */
    uint64_t getSeqNumDeltaSet();

    /**
     * Reset sequence number
     */
    void resetSeqNumDeltaSet();

    /**
     * Get current MigrationClient state.
     */
    MigrationClientState getMigClientState();

    /**
     * Set current Migration Client state.
     */
    void setMigClientState(MigrationClientState newState);

    /**
     * MigrationClient state.
     * Doesn't really need to be atomic, but for safety in future????
     */
    std::atomic<MigrationClientState> migClientState;

    /**
     * SM token which is derived from the set of DLT tokens.
     */
    fds_token_id SMTokenID;

    /**
     * bits per dlt token.
     */
    fds_uint32_t bitsPerDltToken;

    /**
     * Target DLT version for the undergoing SM token migration.
     */
    fds_uint64_t targetDltVersion;

    /**
     * Flag indicating objects in SM token for which this migration
     * client is responsible need to be forwarded to destination SM
     * Does not need to be atomic, because currently it is set under
     * write lock in stor mgr.
     */
    fds_bool_t forwardingIO;

    /**
     * Mutex for dltTokenIDs and filterObjectSet.
     */
    std::mutex migClientLock;

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
     *              1MB can hold 23,831 objects (44 bytes for <objectID+refcnt+(volid+refcnt)>)
     *              1GB can hold 24,443,223 objects...
     *
     * TODO(Sean): Second optimization is how to filter this object lists against the
     *             source SM snapshot.  If filterObjectList is big, then we have to something
     *             to minimize the memory signature.  One way to do it sort both
     *             filterObjectList and snapshot, and iterate like merge sort to get
     *             unique objects.
     */
    std::unordered_map<ObjectID, fpi::CtrlObjectMetaDataSync, ObjectHash> filterObjectSet;

    /**
     * Maintain the message from the destination SM to determine if all
     * sequences are received by the client.  Since the message from the
     * destination SM is asynchronous, the client can receive them
     * out of order.  This is to ensure that all messages are received.
     */
    MigrationSeqNum seqNumFilterSet;

    /**
     * Destination SM node ID.  This is the SM Node ID that's requesting the
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
     * First persistent leveldb snapshot directory.  This is set in the snapshot callback.
     */
    std::string firstPhaseSnapshotDir;

    /**
     * Second persistent leveldb snapshot directory.  This is set in the snapshot callback.
     */
    std::string secondPhaseSnapshotDir;

    /**
     * Maximum number of objects to send in delta set back to the destination SM.
     */
    fds_uint32_t maxDeltaSetSize;

    /**
     * Maintain the sequence number for the delta set of object to be sent
     * from the source SM to destination SM.
     */
    std::atomic<uint64_t> seqNumDeltaSet;

    /**
     * Keep track of oustanding IO request for the MigrationClient.  This is mainly used
     * to prevent the MigrationMgr from aborting and deleteing the Client if there are 
     * still outstanding IO requests pending.
     */
    MigrationTrackIOReqs trackIOReqs;

    /**
     * Will this migration have single phase only?
     */
    bool onePhaseMigration;
};  // class MigrationClient

}  // namespace fds



#endif  // SOURCE_STOR_MGR_INCLUDE_MIGRATIONCLIENT_H_
