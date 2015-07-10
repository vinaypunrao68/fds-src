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
			DmMigrationClientDoneHandler _handle);
    ~DmMigrationClient();

    /*
     * Takes a snapshot of the volume that this client is in charge of,
     * make a list of blobs and generate the delta blob descriptor set,
     * and diffs it against the destination's InitialBlobFilterSet.
     */
    Error handleInitialBlobFilterMsg();

    /**
     * Callback for the async task once done.
     */
    Error handleInitialBlobFilterMsgDone();

    typedef std::unique_ptr<DmMigrationClient> unique_ptr;
    typedef std::shared_ptr<DmMigrationClient> shared_ptr;


    // XXX: only public so we can unit test it
    static Error diffBlobLists(const std::map<int64_t, int64_t>& dest,
                               const std::map<int64_t, int64_t>& source,
                               std::vector<fds_uint64_t>& update_list,
                               std::vector<fds_uint64_t>& delete_list);

    void processDiff();

    /**
     * Called by the handleInitialBlobFilterMsg() after processing the diff to
     * send the delta blobs the destination node.
     */
    void sendCtrlNotifyDeltaBlobs();

    /*
     * TODO Temp function
     */
    Error generateRandomDeltaBlobs(std::vector<fpi::CtrlNotifyDeltaBlobsPtr> &blobsMsg);


 private:
    /**
     * Reference to the Data Manager.
     */
    DataMgr& dataMgr;
    DmIoReqHandler* DmReqHandler;

    /**
     * Local copies
     */
    NodeUuid mySvcUuid;
    NodeUuid destDmUuid;
    fds_volid_t volID;
    fpi::CtrlNotifyInitialBlobFilterSetMsgPtr& ribfsm;
    std::vector<fpi::CtrlNotifyDeltaBlobsPtr> myBlobMsgs;

    /**
     * Snapshot used for diff.
     */
    Catalog::catalog_roptions_t opts;


    /**
     * Callback to talk to DM Migration Manager
     */
    DmMigrationClientDoneHandler migrDoneHandler;
    friend class DmMigrationMgr;

};  // DmMigrationClient


}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONCLIENT_H_
