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
										std::vector<fpi::BlobFilterSetEntry>& _filterSet,
										DmMigrationClientDoneHandler _handle)
    : DmReqHandler(_DmReqHandle), migrDoneHandler(_handle)
{
	mySvcUuid = boost::make_shared<NodeUuid> (_myUuid);
	destDmUuid = boost::make_shared<NodeUuid> (_destDmUuid);
	volID = boost::make_shared<fds_volid_t> (_volId);
	blob_filter_set = boost::make_shared<std::vector<fpi::BlobFilterSetEntry>> (_filterSet);
}

DmMigrationClient::~DmMigrationClient()
{
}


}  // namespace fds


