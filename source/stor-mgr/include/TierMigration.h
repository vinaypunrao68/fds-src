/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_TIERMIGRATION_H_
#define SOURCE_STOR_MGR_INCLUDE_TIERMIGRATION_H_

#include <SmIo.h>
#include <object-store/TieringConfig.h>

namespace fds {

/*
 * Class responsible for migrating data between tiers:
 * object promotion from HDD to flash and writeback from flash to HDD.
 * The caller drives when/and what to migrate; this class manages
 * qos work items, and does bookkeeping
 */
class SmTierMigration {
  public:
    explicit SmTierMigration(SmIoReqHandler *data_store);
    ~SmTierMigration();

    typedef std::unique_ptr<SmTierMigration> unique_ptr;

    /**
     * Handles notification about successful PUT to flash
     * by hybrid volume
     */
    Error notifyHybridVolFlashPut(const ObjectID& oid);

    /**
     * Over-write tier config with a new config
     */
    void setTierConfig(const TieringParams& conf);

    /**
     * Returns flash full threshold config
     */
    inline fds_uint32_t flashFullThreshold() const {
        return tierConfig.flashFullThreshold;
    }

  private:
    /// creates QoS request for writeback work
    void createWritebackReq();

    /// tiering configurable parameters
    TieringParams tierConfig;

    /// for enqueueing to QoS queues, does not own
    SmIoReqHandler *dataStoreReqHandler;

    /**
     * Handling write-back to HDD of objects that were
     * written to SSD. Collects a set of objects to write-back
     * into QoS request, and once max number of objects are,
     * QoS request is put to QoS queue and a new QoS request is
     * created.
     */
    SmIoMoveObjsToTier* writeBackReq;
    fds_mutex writeBackLock;
};

}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_TIERMIGRATION_H_
