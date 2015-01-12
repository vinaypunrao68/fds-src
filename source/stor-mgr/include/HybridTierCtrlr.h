/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_HYBRIDTIERCTRLR_H_
#define SOURCE_STOR_MGR_INCLUDE_HYBRIDTIERCTRLR_H_

#include <vector>
#include <fds_types.h>
#include <SmIo.h>

namespace fds {

/* Forward declarations */
class CommonModuleProviderIf;
class fds_threadpool;
class SmIoReqHandler;
class FdsTimerTask;
typedef boost::shared_ptr<FdsTimerTask> FdsTimerTaskPtr;

/**
* @brief Class responsible for enforching hybrid tiering policy
* NOTE: This implementation is valid for beta-2 timeframe.
*/
struct HybridTierCtrlr {
    HybridTierCtrlr(CommonModuleProviderIf* modProvider,
                    SmIoReqHandler* storMgr,
                    SmDiskMap::ptr diskMap);
    void start();
    void stop();

    void moveToNextToken();
    void snapToken();
    void constructTierMigrationList();
    void snapTokenCb(const Error& err,
                     SmIoSnapshotObjectDB* snapReq,
                     leveldb::ReadOptions& options,
                     leveldb::DB* db);
    void moveObjsToTierCb(const Error& e,
                          SmIoMoveObjsToTier *req);
 protected:
    static uint32_t BATCH_SZ;
    static uint32_t FREQUENCY;

    CommonModuleProviderIf* modProvider_;
    fds_threadpool *threadpool_;
    SmIoReqHandler* storMgr_;
    SmDiskMap::ptr diskMap_;
    FdsTimerTaskPtr runTask_;
    SmIoSnapshotObjectDB snapRequest_;
    std::vector<fds_token_id> tokenList_;
    int curTokenIdx_;
    std::unique_ptr<SMTokenItr> tokenItr_;
    SmIoMoveObjsToTier moveTierRequest_;
    uint64_t hybridMoveTs_;
};
} // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_HYBRIDTIERCTRLR_H_
