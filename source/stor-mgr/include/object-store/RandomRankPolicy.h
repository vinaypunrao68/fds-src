/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_RANDOMRANKPOLICY_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_RANDOMRANKPOLICY_H_

#include <fds_types.h>
#include <set>
#include <mutex>
#include <condition_variable>

#include <persistent-layer/dm_io.h>

#include <object-store/RankEngine.h>
#include <ObjMeta.h>
#include <SmIo.h>

namespace fds {
class RandomRankPolicy : public RankEngine {
  public:
    explicit RandomRankPolicy(SmIoReqHandler *_dataStore,
                              uint32_t pct);
    ~RandomRankPolicy();

    virtual void getObjectsToPromote(fds_uint32_t maxSize,
                                     PromotionSet& oidSet);

    virtual fds_bool_t isObjectDemotable(const ObjectID& oid);

    virtual void notifyDataPath(fds_io_op_t opType,
                                const ObjectID& oid,
                                diskio::DataTier tier);

  private:
    /**
     * Callback function from the snapshot request that will determine
     * if a object is needs to be added to the promotion set.
     */
    void getPromotionObjectIds(const Error& error,
                               SmIoSnapshotObjectDB* snapReq,
                               leveldb::ReadOptions& options,
                               leveldb::DB *db);

    /**
     * Choose set of object ids in this token id.
     */
    void getObjectIds(fds_token_id tokenId);

    /**
     * Since the snapshot is done in another thread's context, need a
     * set of synchronization and a predicate to coordinate the snapshot request.
     */
    std::mutex snapRequestMutex;
    std::condition_variable snapRequestCondVar;
    bool snapRequestCompleted;

    /**
     * This pointer is set to the caller's promotion set when request is
     * made.
     */
    PromotionSet *promoSetPtr;

    /**
     * Maximum number of size for promotion set.
     */
    fds_uint32_t maxSize;

    /**
     * Object data store handler.  Set during the initialization.
     */
    SmIoReqHandler *dataStore;

    /**
     * Request to snapshot index DB for a given token.
     */
    SmIoSnapshotObjectDB snapRequest;

    /**
     * Random number generator.
     */
    std::default_random_engine randGen;

    /**
     * Percentage of the object chosen.
     */
    uint32_t selectionPct;

    /**
     * Using uniform distribution to give equal chance for all ObjectIDs requested
     * for.... I think that's the case...
     * [0..99], everything below the selectionPct is chosen.
     */
    std::uniform_int_distribution<uint32_t> randDemotePromoteObj;

    /**
     * Second uniform distribution for token id.  We randomly select token ID
     * for snapshot.
     */
    std::uniform_int_distribution<uint32_t> randTokenId;
};
}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_RANDOMRANKPOLICY_H_
