/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONCLIENT_H_
#define SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONCLIENT_H_

namespace fds {

// Forward declaration.
class DmIoReqHandler;
class DataMgr;

/**
 * Callback for once client is finished with migration.
 */
typedef std::function<void (fds_uint64_t clientId,
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

    typedef std::unique_ptr<DmMigrationClient> unique_ptr;
    typedef std::shared_ptr<DmMigrationClient> shared_ptr;


    // XXX: only public so we can unit test it
    static Error diffBlobLists(const std::map<int64_t, int64_t>& dest,
                               const std::map<int64_t, int64_t>& source,
                        std::vector<fds_uint64_t>& update_list,
                        std::vector<fds_uint64_t>& delete_list);

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

    /**
     * Callback to talk to DM Migration Manager
     */
    DmMigrationClientDoneHandler migrDoneHandler;

};  // DmMigrationClient


}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONCLIENT_H_
