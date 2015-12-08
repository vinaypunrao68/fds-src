/*
 * Copyright 2015 Formation Data Systems, Inc.
 */


#ifndef SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONBASE_H_
#define SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONBASE_H_
#include <functional>
#include <string>

namespace fds {

extern size_t sizeOfData(fpi::CtrlNotifyDeltaBlobDescMsgPtr &msg);
extern size_t sizeOfData(fpi::CtrlNotifyDeltaBlobsMsgPtr &msg);

/**
 * Common base class that provides functionality that DmMigrationMgr, Executor, and client
 * may use.
 */
class DmMigrationBase {
public:
    using migrationCb = std::function<void(const Error& e)>;
    DmMigrationBase() = default;
    explicit DmMigrationBase(int64_t migrationId);
	virtual ~DmMigrationBase() {}
    /**
     * Response handler - no-op for OK, otherwise fail migration.
     */
    void dmMigrationCheckResp(std::function<void()>,
    						  std::function<void()>,
    						  EPSvcRequest*,
    						  const Error&,
							  boost::shared_ptr<std::string>);
    virtual std::string logString() const;

protected:
    /* Id to identify migration. For now this can be the dmt version */
    int64_t     migrationId;
    std::string logStr;

};

} // namespace fds

#endif // SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONBASE_H_
