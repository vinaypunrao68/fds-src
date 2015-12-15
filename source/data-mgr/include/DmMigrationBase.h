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

using migrationCb = std::function<void(const Error& e)>;
/**
 * Common base class that provides functionality that DmMigrationMgr, Executor, and client
 * may use.
 */
class DmMigrationBase {
public:
    DmMigrationBase(int64_t migrationId, DataMgr& _dataMgr);
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

    void asyncMsgPassed();
    void asyncMsgFailed();
    void asyncMsgIssued();
    void waitForAsyncMsgs();

protected:
    /* Id to identify migration. For now this can be the dmt version */
    int64_t                 migrationId;
    DataMgr&                dataMgr;
    MigrationTrackIOReqs    trackIOReqs;
    std::string             logStr;
};

} // namespace fds

#endif // SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONBASE_H_
