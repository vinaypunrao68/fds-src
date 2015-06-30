/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DataMgr.h>
#include <DmMigrationClient.h>

namespace fds {

DmMigrationClient::DmMigrationClient(DmIoReqHandler* _DmReqHandle,
										DataMgr& _dataMgr,
										const NodeUuid& _myUuid,
										NodeUuid& _destDmUuid,
										fpi::CtrlNotifyInitialBlobFilterSetMsgPtr& _ribfsm,
										DmMigrationClientDoneHandler _handle)
    : DmReqHandler(_DmReqHandle), migrDoneHandler(_handle), mySvcUuid(_myUuid),
	  destDmUuid(_destDmUuid), dataMgr(_dataMgr), ribfsm(_ribfsm)
{

}

DmMigrationClient::~DmMigrationClient()
{

}


}  // namespace fds


