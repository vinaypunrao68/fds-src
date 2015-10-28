/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONCLIENT_H_
#define SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONCLIENT_H_

#include <dmhandler.h>

namespace fds {

// Forward declaration.
class DmIoReqHandler;
class DataMgr;

/**
 * Callback for once client is finished with migration.
 */
typedef std::function<void (fds_volid_t clientId,
                            const Error& error)> DmMigrationClientDoneHandler;

using incrementCountFunc = std::function<void()>;

class DmMigrationClient : public DmMigrationBase {
  public:
    explicit DmMigrationClient(DmIoReqHandler* DmReqHandle,
    		DataMgr& _dataMgr,
    		const NodeUuid& _myUuid,
			NodeUuid& _destDmUuid,
			fpi::CtrlNotifyInitialBlobFilterSetMsgPtr& _ribfsm,
			DmMigrationClientDoneHandler _handle,
            uint64_t _maxDeltaBlobs,
            uint64_t _maxDeltaBlobDesc);
    ~DmMigrationClient();

    /**
     * Takes a snapshot of the volume that this client is in charge of,
     * make a list of blobs and generate the delta blob descriptor set,
     * and diffs it against the destination's InitialBlobFilterSet.
     */
    Error processBlobFilterSet(incrementCountFunc inTracker);

    Error processBlobFilterSet2();

    typedef std::unique_ptr<DmMigrationClient> unique_ptr;
    typedef std::shared_ptr<DmMigrationClient> shared_ptr;

    /**
     * Whether or not I/O to this volume needs to be forwarded
     * as part of Active Migration.
     * Input: dmtVersion - the version of DMT that the commit log belongs to
     */
    fds_bool_t shouldForwardIO(fds_uint64_t dmtVersion);

    /* Forwarding Modifiers */
    void turnOnForwarding();
    void turnOffForwarding();
    void turnOffForwardingInternal(); // No sending of finish messages

    /**
     * Sends a msg to say that we're done with forwarding.
     */
    Error sendFinishFwdMsg();

    /**
     * Forward the committed blob to the destination side.
     * Note: Because DmMigrationMgr can't check forward I/O at this point,
     * this method will only forward if the internal atomic var says it's ok.
     */
    Error forwardCatalogUpdate(DmIoCommitBlobTx *commitBlobReq,
    						   blob_version_t blob_version,
							   const BlobObjList::const_ptr& blob_obj_list,
							   const MetaDataList::const_ptr& meta_list);


    /**
     * Callback to ensure no error from forwarding.
     * Otherwise, fail the migration for now.
     */
    void fwdCatalogUpdateMsgResp(EPSvcRequest* req,
                                 const Error& error,
                                 boost::shared_ptr<std::string> payload);

    // XXX: only public so we can unit test it
    static Error diffBlobLists(const std::map<std::string, int64_t>& dest,
                               const std::map<std::string, int64_t>& source,
                               std::vector<std::string>& update_list,
                               std::vector<std::string>& delete_list);

    /**
     * Response handler - no-op for OK, otherwise fail migration.
     */
    void dmMigrationCheckResp(EPSvcRequest*, const Error&, boost::shared_ptr<std::string>);

    // Called by MigrationMgr to clean up any ongoing residue due to migration
    void abortMigration();

 private:
    /**
     * Reference to the Data Manager.
     */
    DataMgr& dataMgr;
    DmIoReqHandler* DmReqHandler;

    /**
     * local svc uuid
     */
    NodeUuid mySvcUuid;

    /**
     * destination dm svc uuid.
     */
    NodeUuid destDmUuid;

    /**
     * volume ID that the client manages.
     */
    fds_volid_t volId;

    /**
     * Number of blob descriptors before sending to destination DM.
     */
    uint64_t maxNumBlobDescs;

    /**
     * Number of blob descriptors before sending to destination DM.
     */
    uint64_t maxNumBlobs;

    /**
     * Local copy of the dmtVersion undergoing migration
     */
    fds_uint64_t dmtVersion;

    /**
     * Maintain the sequence number for the delta set of blob offset set to
     * the destination DM.
     */
    std::atomic<uint64_t> seqNumBlobs;

    /**
     * Maintain the sequence number for the delta set of blob offset set to
     * the destination DM.
     */
    std::atomic<uint64_t> seqNumBlobDescs;

    /**
     * shared pointer to the initial blob filter set message
     */
    fpi::CtrlNotifyInitialBlobFilterSetMsgPtr& ribfsm;

    /**
     * Snapshot used for diff.
     */
    Catalog::MemSnap snap_;

    /**
     * Callback to talk to DM Migration Manager
     */
    DmMigrationClientDoneHandler migrDoneHandler;
    friend class DmMigrationMgr;

    /**
     * From list of blob update and delete list, generate and send the
     * delta blob offset list and delta blob descriptor list.
     */
    Error generateBlobDeltaSets(const std::vector<std::string>& updateBlobs,
                                const std::vector<std::string>& deleteBlobs);

    /**
     * Generate list of blobs to update or delete.
     */
    Error processBlobDiff();

    /**
     * Generate delta set based on update blob ids and delete ids.
     */
    Error generateUpdateBlobDeltaSets(const std::vector<std::string>& deleteBlobs);
    Error generateDeleteBlobDeltaSets(const std::vector<std::string>& updateBlobs);

    /**
     * Send blobs msg and blob descs msg.
     */
    Error sendDeltaBlobs(fpi::CtrlNotifyDeltaBlobsMsgPtr& blobsMsg);
    Error sendDeltaBlobDescs(fpi::CtrlNotifyDeltaBlobDescMsgPtr& blobDescMsg);

    /**
     * sequence number apis for blobs.
     * When getSeqNumBlobs() is called, it will automatically increment.
     */
    uint64_t getSeqNumBlobs();
    void resetSeqNumBlobs();

    /**
     * Used between processBlobFilterSet part 1 and 2
     */
    DmCommitLog::ptr commitLog;

    /**
     * sequence number apis for blobs.
     * When getSeqNumBlobDesc() is called, it will automatically increment.
     */
    uint64_t getSeqNumBlobDescs();
    void resetSeqNumBlobDescs();

    /**
     * Whether or not we're forwarding I/O during Active Migration
     */
    std::atomic<fds_bool_t> forwardingIO;

    // Used for abort cleanup
    fds_mutex  ssTakenScopeLock;
    fds_bool_t snapshotTaken;

    // Function pointer for incrementing count per message sent
    incrementCountFunc trackerFunc;

};  // DmMigrationClient


}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONCLIENT_H_
