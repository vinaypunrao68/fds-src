/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#include <DataMgr.h>
#include <DmMigrationBase.h>

namespace fds {

DmMigrationBase::DmMigrationBase(int64_t migrationId)
{
    this->migrationId = migrationId;
    std::stringstream ss;
    ss << " migrationid: " << migrationId << " ";
    logStr = ss.str();
}

std::string DmMigrationBase::logString() const
{
    return logStr;
}

void
DmMigrationBase::dmMigrationCheckResp(std::function<void()> abortFunc,
										std::function<void()> passFunc,
										EPSvcRequest *req,
										const Error& error,
										boost::shared_ptr<std::string> payload)
{
	LOGMIGRATE << logString()
        << "Request : " << req << " Received response with error: " << error;
	if (!error.ok()) {
		abortFunc();
	} else {
		passFunc();
	}
}

} // namespace fds
