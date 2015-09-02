/*
 * Copyright 2015 Formation Data Systems, Inc.
 */


#ifndef SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONBASE_H_
#define SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONBASE_H_
#include <functional>

namespace fds {

// This is a hack because somehow the setTimeoutMs() doesn't like reading from a uint...
// It only likes hard-coded numbers.
#define DMMIGRATION_TIMEOUT 3000
class DmMigrationBase {
public:
    using migrationCb = std::function<void(const Error& e)>;
	virtual ~DmMigrationBase() {}
    /**
     * Response handler - no-op for OK, otherwise fail migration.
     */
    void dmMigrationCheckResp(std::function<void()>,
    						  EPSvcRequest*,
    						  const Error&,
							  boost::shared_ptr<std::string>);

};

} // namespace fds

#endif // SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONBASE_H_
