/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#include <object-store/RandomRankPolicy.h>

namespace fds {

/**
 * This is the Version1 of the Object Rank Policy.
 *
 * This algorithm randomly choose a ObjectID for promotion or
 * demotion in a hybrid tier model.
 *
 * If other algorithms can't do better than this algorithm, it's
 * not worth pursuing it.
 */


RandomRankPolicy::RandomRankPolicy(SmIoReqHandler *_dataStore,
                                   uint32_t pct = 50)
    : dataStore(_dataStore),
      selectionPct(pct),
      randGen(std::chrono::system_clock::now().time_since_epoch().count()),
      randDemotePromoteObj(0, 99),  // [0..99] percent
      randTokenId(0, 0xff),  // [0..255] token ID
      snapRequestCompleted(false)
{
    snapRequest.io_type = FDS_SM_SNAPSHOT_TOKEN;
    snapRequest.smio_snap_resp_cb = std::bind(&RandomRankPolicy::getPromotionObjectIds,
                                              this,
                                              std::placeholders::_1,
                                              std::placeholders::_2,
                                              std::placeholders::_3,
                                              std::placeholders::_4);
    maxSize = 0;
    promoSetPtr = NULL;
}

RandomRankPolicy::~RandomRankPolicy()
{
}


/**
 * Request snapshot of index db.  The call back function will notify
 * when list of object ids are returned.
 */
void
RandomRankPolicy::getObjectIds(fds_token_id tokenId)
{
    Error err(ERR_OK);

    LOGDEBUG << "Requesting getting list of promotional objects";
    snapRequest.token_id = tokenId;
    err = dataStore->enqueueMsg(FdsSysTaskQueueId, &snapRequest);
    if (!err.ok()) {
        LOGERROR << "Failed to enqueue snapshot request: err " << err;
        return;
    }

    /**
     * Wait until the list of the objects for promotion is obtained.
     */
    LOGDEBUG << "Waiting for list of promotional object";
    std::unique_lock<std::mutex> myLock(snapRequestMutex);
    snapRequestCondVar.wait(myLock,
                            [this]{ return this->snapRequestCompleted; });
    // reset the snapshot request state.
    snapRequestCompleted = false;
}

/**
 * This is the gut of the code that gathers the list of objects IDs to be
 * promoted.
 */
void
RandomRankPolicy::getPromotionObjectIds(const Error& error,
                                        SmIoSnapshotObjectDB* snapReq,
                                        leveldb::ReadOptions& options,
                                        leveldb::DB *db)
{
    Error err(ERR_OK);

    /**
     * Iterate through the level db and randomly select OID.
     */
    leveldb::Iterator* it = db->NewIterator(options);
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        ObjectID id(it->key().ToString());

        /**
         * Randomly choose the object.  If it falls below the
         * pct threshold, add to the set.
         */
        if (randDemotePromoteObj(randGen) < selectionPct) {
            promoSetPtr->push_back(id);
        }

        /** If we've added enough, time to return.
         */
        if (promoSetPtr->size() >= maxSize) {
            break;
        }
    }

    /**
     * At this point, we have [0..maxSize] object ids.
     */

    /**
     * Now release the snapshot, since we no longer need it.
     */
    db->ReleaseSnapshot(options.snapshot);

    LOGDEBUG << "Completed getting list of PromotionObjects";

    /**
     * Now signal the snapshot thread that the promotion list is complete.
     */
    {
        std::lock_guard<std::mutex> myLock(snapRequestMutex);
        snapRequestCompleted = true;
        snapRequestCondVar.notify_all();
    }
}

/**
* Determine and return a set of objects to be promoted.
* @param max_size The maxiumum number of objects to find for promotion.
* Note that max is just an upper bound.
* @param oidSet A set of ObjectIDs returned to the caller.
*
* @returns PromotionSet of objects to be promoted
*/
void
RandomRankPolicy::getObjectsToPromote(fds_uint32_t maxSize,
                                      PromotionSet& oidSet)
{
    /**
     * Randomly choose token.
     */
    fds_token_id tokenId = randTokenId(randGen);

    /**
     * Set the pointer to the oidSet specified by the caller before
     * requesting snapshot.  The CB function from the snapshot
     * will fill the oidSet.
     */
    promoSetPtr = &oidSet;

    /**
     * Also, set the max size;
     */
    this->maxSize = maxSize;

    /**
     * Get set of object ids based on the randomly chose token.
     */
    getObjectIds(tokenId);

    /** After getting the token, reset the set ptr and max size.
     */
    promoSetPtr = NULL;
    this->maxSize = 0;
    return;
}

/**
* Determine if an object should be demoted or not. This will be called
* by the scavenger during garbage collection to determine if an object
* should be removed from SSD or not.
*
* @param oid An objectID of the object to check on
*
* @returns true if the object should be demoted, false otherwise
*/
fds_bool_t
RandomRankPolicy::isObjectDemotable(const ObjectID& oid)
{
    /**
     * Return true if the random number generator is below the selection
     * pct.  Range of number is [0..99], so if selectionPct is:
     *   0 - nothing selected
     *   100 - everything selected
     *   [1..99]  -- something in between
     */
    return (randDemotePromoteObj(randGen) < selectionPct) ? true : false;
}

/**
* Gets called every time an IO occurs to alert the rank engine to update
* data structures, etc.
*/
void
RandomRankPolicy::notifyDataPath(fds_io_op_t opType,
                                 const ObjectID& oid,
                                 diskio::DataTier tier)
{
    // Intentionally does nothing.
    return;
}

}  // namespace fds
