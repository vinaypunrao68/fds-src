/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DataMgr.h>
#include <DmMigrationBase.h>

namespace fds {

void
DmMigrationBase::dmMigrationCheckResp(std::function<void()> abortFunc,
									  EPSvcRequest *req,
									  const Error& error,
									  boost::shared_ptr<std::string> payload)
{
	LOGMIGRATE << "Received response with error: " << error;
	if (!error.ok()) {
		abortFunc();
	}
}

} // namespace fds
