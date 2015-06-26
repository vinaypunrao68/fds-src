/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DmIoReq.h>
#include <DmMigrationClient.h>

namespace fds {

DmMigrationClient::DmMigrationClient(DmIoReqHandler* _DmReqHandle,
										const NodeUuid& _myUuid,
										NodeUuid& _destDmUuid,
										fds_volid_t& _volId,
										// std::vector<fpi::BlobFilterSetEntry>& _filterSet,
										std::vector<fpi::BlobFilterSetEntryPtr>& _filterSet,
										DmMigrationClientDoneHandler _handle)
    : DmReqHandler(_DmReqHandle), migrDoneHandler(_handle), mySvcUuid(_myUuid),
	  destDmUuid(_destDmUuid), volID(_volId)
{
	blob_filter_set =
			boost::make_shared<std::vector<fpi::BlobFilterSetEntryPtr>> (_filterSet);
}

DmMigrationClient::~DmMigrationClient()
{

}


}  // namespace fds


