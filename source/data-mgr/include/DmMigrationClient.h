/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONCLIENT_H_
#define SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONCLIENT_H_

namespace fds {

// Forward declaration.
class DmIoReqHandler;

/**
 * Callback for once client is finished with migration.
 */
typedef std::function<void (fds_uint64_t clientId,
                            const Error& error)> DmMigrationClientDoneHandler;

class DmMigrationClient {
  public:
    explicit DmMigrationClient(DmIoReqHandler* DmReqHandle,
    		const NodeUuid& _myUuid,
			NodeUuid& _destDmUuid,
			fds_volid_t& _volId,
			// std::vector<fpi::BlobFilterSetEntry>& _filterSet,
			std::vector<fpi::BlobFilterSetEntryPtr>& _filterSet,
			DmMigrationClientDoneHandler _handle);
    ~DmMigrationClient();

    typedef std::unique_ptr<DmMigrationClient> unique_ptr;
    typedef std::shared_ptr<DmMigrationClient> shared_ptr;

  private:
    DmIoReqHandler* DmReqHandler;

    /**
     * Local copies
     */
    NodeUuid mySvcUuid;
    NodeUuid destDmUuid;
    fds_volid_t volID;
    // boost::shared_ptr <std::vector<fpi::BlobFilterSetEntry>> blob_filter_set;
    // boost::shared_ptr <std::vector<fpi::BlobFilterSetEntryPtr>> blob_filter_set;
    boost::shared_ptr<std::vector<fpi::BlobFilterSetEntryPtr>> blob_filter_set;

    /**
     * Callback to talk to DM Migration Manager
     */
    DmMigrationClientDoneHandler migrDoneHandler;

};  // DmMigrationClient


}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONCLIENT_H_

