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

class DmMigrationClient {
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
    Error processBlobFilterSet();

    /**
     * Callback for the async task once done.
     */
    Error processBlobFilterSetCb();

    typedef std::unique_ptr<DmMigrationClient> unique_ptr;
    typedef std::shared_ptr<DmMigrationClient> shared_ptr;


    // XXX: only public so we can unit test it
    static Error diffBlobLists(const std::map<std::string, int64_t>& dest,
                               const std::map<std::string, int64_t>& source,
                               std::vector<std::string>& update_list,
                               std::vector<std::string>& delete_list);

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
     * sequence number apis for blobs.
     * When getSeqNumBlobDesc() is called, it will automatically increment.
     */
    uint64_t getSeqNumBlobDescs();
    void resetSeqNumBlobDescs();

};  // DmMigrationClient


}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONCLIENT_H_
