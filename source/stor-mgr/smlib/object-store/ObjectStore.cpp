/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <map>
#include <fiu-control.h>
#include <fiu-local.h>

#include <fdsp_utils.h>
#include <ObjectId.h>
#include <fds_process.h>
#include <PerfTrace.h>
#include <object-store/TokenCompactor.h>
#include <object-store/ObjectStore.h>
#include <sys/statvfs.h>
#include <utility>
#include <object-store/TieringConfig.h>
#include <include/util/disk_utils.h>

namespace fds {

template <typename T, typename Cb>
static std::unique_ptr<TrackerBase<fds_uint16_t>>
create_tracker(Cb&& cb, std::string event, fds_uint32_t win = 0, fds_uint32_t th = 0) {
    static std::string const prefix("fds.sm.disk_event_threshold.");

    size_t window = g_fdsprocess->get_fds_config()->get<fds_uint32_t>(prefix + event + ".window", win);
    size_t threshold = g_fdsprocess->get_fds_config()->get<fds_uint32_t>(prefix + event + ".threshold", th);

    return std::unique_ptr<TrackerBase<fds_uint16_t>>
            (new TrackerMap<Cb, fds_uint16_t, T>(std::forward<Cb>(cb), window, threshold));
}

extern std::string logString(const FDS_ProtocolInterface::CtrlObjectMetaDataPropagate& msg);

ObjectStore::ObjectStore(const std::string &modName,
                         SmIoReqHandler *data_store,
                         StorMgrVolumeTable* volTbl,
                         StartResyncFnObj fn,
                         DiskChangeFnObj dcFn)
        : Module(modName.c_str()),
          volumeTbl(volTbl),
          requestResyncFn(fn),
          conf_verify_data(true),
          diskMap(new SmDiskMap("SM Disk Map Module",
                                std::move(dcFn))),
          dataStore(new ObjectDataStore("SM Object Data Storage",
                                        data_store,
                                        std::bind(&ObjectStore::updateMediaTrackers,
                                                  this,
                                                  std::placeholders::_1,
                                                  std::placeholders::_2,
                                                  std::placeholders::_3))),
          metaStore(new ObjectMetadataStore("SM Object Metadata Storage Module",
                                            std::bind(&ObjectStore::updateMediaTrackers,
                                                      this,
                                                      std::placeholders::_1,
                                                      std::placeholders::_2,
                                                      std::placeholders::_3))),
          tierEngine(new TierEngine("SM Tier Engine",
                                    TierEngine::FDS_RANDOM_RANK_POLICY,
                                    diskMap, data_store)),
          SMCheckCtrl(new SMCheckControl("SM Checker",
                                         diskMap, data_store)),
          currentState(OBJECT_STORE_INIT),
          lastCapacityMessageSentAt(0)
{
}

ObjectStore::~ObjectStore() {
    dataStore.reset();
    metaStore.reset();
}


void ObjectStore::setUnavailable() {
    GLOGDEBUG << "Setting ObjectStore state to OBJECT_STORE_UNAVAILABLE. This should block future IOs";
    currentState = OBJECT_STORE_UNAVAILABLE;
}

float_t ObjectStore::getUsedCapacityAsPct() {

    // Error injection points
    // Causes the method to return DISK_CAPACITY_ERROR_THRESHOLD capacity
    fiu_do_on("sm.objstore.get_used_capacity_error", \
            fiu_disable("sm.objstore.get_used_capacity_warn"); \
            fiu_disable("sm.objstore.get_used_capacity_alert"); \
            LOGDEBUG << "Err inection: returning max used disk capacity as " \
                     << DISK_CAPACITY_ERROR_THRESHOLD; \
            return DISK_CAPACITY_ERROR_THRESHOLD; );

    // Causes the method to return DISK_CAPACITY_ALERT_THRESHOLD + 1 % capacity
    fiu_do_on("sm.objstore.get_used_capacity_alert", \
              fiu_disable("sm.objstore.get_used_capacity_warn"); \
              fiu_disable("sm.objstore.get_used_capacity_error"); \
              LOGDEBUG << "Err injection: returning max used disk capacity as " \
                       << DISK_CAPACITY_ALERT_THRESHOLD + 1; \
              return DISK_CAPACITY_ALERT_THRESHOLD + 1; );

    // Causes the method to return DISK_CAPACITY_WARNING_THRESHOLD + 1 % capacity
    fiu_do_on("sm.objstore.get_used_capacity_warn", \
              fiu_disable("sm.objstore.get_used_capacity_error"); \
              fiu_disable("sm.objstore.get_used_capacity_alert"); \
              LOGDEBUG << "Err injection: returning max used disk capacity as " \
                       << DISK_CAPACITY_WARNING_THRESHOLD + 1; \
              return DISK_CAPACITY_WARNING_THRESHOLD + 1; );

    float_t max = 0;
    // For disks
    for (auto diskId : diskMap->getDiskIds()) {
        // Get the (used, total) pair
        DiskUtils::capacity_tuple capacity = diskMap->getDiskConsumedSize(diskId);

        // Check to make sure we've got good data from the stat call
        if (capacity.first == 0 || capacity.second == 0) {
            // If we don't just return 0
            LOGDEBUG << "Found disk used capacity of zero, possible error. DiskID = " << diskId
                        << ". Disk path = " << diskMap->getDiskPath(diskId) << ". If this is an SSD drive"
                        << " and you have not yet written data to the system, this message is OK.";
            break;
        }
        float_t pct_used = ((capacity.first * 1.) / capacity.second) * 100;

        // We want to log which disk is too full here
        if (pct_used >= DISK_CAPACITY_ERROR_THRESHOLD &&
                lastCapacityMessageSentAt < DISK_CAPACITY_ERROR_THRESHOLD) {
            LOGERROR << "ERROR: Disk at path " << diskMap->getDiskPath(diskId)
                     << " is consuming " << pct_used << "Space, which is more than the error threshold of "
                     << DISK_CAPACITY_ERROR_THRESHOLD;
            lastCapacityMessageSentAt = DISK_CAPACITY_ERROR_THRESHOLD;
        } else if (pct_used > DISK_CAPACITY_WARNING_THRESHOLD &&
            lastCapacityMessageSentAt < DISK_CAPACITY_WARNING_THRESHOLD) {
            LOGWARN << "WARNING: Disk at path " << diskMap->getDiskPath(diskId)
                    << " is consuming " << pct_used << " space, which is more than the warning threshold of "
                    << DISK_CAPACITY_WARNING_THRESHOLD;
            lastCapacityMessageSentAt = DISK_CAPACITY_WARNING_THRESHOLD;
        } else if (pct_used > DISK_CAPACITY_ALERT_THRESHOLD &&
                   lastCapacityMessageSentAt < DISK_CAPACITY_ALERT_THRESHOLD) {
            LOGNORMAL << "ALERT: Disk at path " << diskMap->getDiskPath(diskId)
                      << " is consuming " << pct_used << " space, which is more than the alert threshold of "
                      << DISK_CAPACITY_ALERT_THRESHOLD;
            lastCapacityMessageSentAt = DISK_CAPACITY_ALERT_THRESHOLD;
        } else {
            // If the used pct drops below alert levels reset so we resend the message when
            // we re-hit this condition
            if (pct_used < DISK_CAPACITY_WARNING_THRESHOLD) {
                lastCapacityMessageSentAt = 0;
            } else if (pct_used < DISK_CAPACITY_ALERT_THRESHOLD) {
                lastCapacityMessageSentAt = DISK_CAPACITY_WARNING_THRESHOLD;
            } else if (pct_used < DISK_CAPACITY_ERROR_THRESHOLD) {
                lastCapacityMessageSentAt = DISK_CAPACITY_ALERT_THRESHOLD;
            }
        }

        if (pct_used > max) {
            max = pct_used;
        }
    }

    return max;
}


void
ObjectStore::initObjectStoreMediaErrorHandlers() {
    // callback for disk failure notification
    LOGDEBUG << "Initializing Object store media error handlers";
    typedef std::function <void(DiskId& diskId, const diskio::DataTier& tier)> OnlineDiskFailureFnObj;
    struct cb {
        diskio::DataTier tier;
        OnlineDiskFailureFnObj fnObj;
        void operator()(fds_uint16_t diskId,
                        size_t events) const {
            LOGERROR << "Disk " << diskId << " on tier " << tier
                     << " saw too many errors; declaring disk failed";
            /**
             * This callback will be called when the timer expires and
             * the number of events fed is greater than a given threshhold.
             */

             fnObj(diskId, tier);
        }
        explicit cb(diskio::DataTier dataTier, OnlineDiskFailureFnObj fn=OnlineDiskFailureFnObj()):
                                                                tier(dataTier), fnObj(std::move(fn)) {}
    };

    // Write/read HDD error handler (3 within 5 minutes will trigger)
    // we will record both read and write errors as write errors
    objStoreDiskMediaTracker.register_event(
                               ERR_DISK_WRITE_FAILED,
                               create_tracker<std::chrono::minutes>(cb(diskio::diskTier,
                                                                       std::bind(&::fds::ObjectStore::handleOnlineDiskFailures,
                                                                                 this,
                                                                                 std::placeholders::_1,
                                                                                 std::placeholders::_2)),
                                                                    "hdd_fail", 5, 3));
    // Write/Read SSD error handler (3 within 5 minutes will trigger)
    // we will record both read and write errors as write errors
    objStoreFlashMediaTracker.register_event(
                               ERR_DISK_WRITE_FAILED,
                               create_tracker<std::chrono::minutes>(cb(diskio::flashTier,
                                                                       std::bind(&::fds::ObjectStore::handleOnlineDiskFailures,
                                                                                 this,
                                                                                 std::placeholders::_1,
                                                                                 std::placeholders::_2)),
                                                                    "ssd_fail", 5, 3));
}

/**
 * This method is called when a certain number of disk read/write
 * failures are oberserved with-in a given time period. When this
 * happens, the disk is marked a bad and preparations are done to
 * recreate all the data on the failed disk from other replicas in
 * the domain.
 *
 * - First, a check is done for the minimum number of disks present
 *   in the node.
 * - Then the disk is removed from all disk book keeping data structs.
 * - After that recomputation and reassignment of smTokens are done
 *   in order to assign the smTokens residing on the failed disk to
 *   online disks.
 * - Once all is done, a request to resync data is posted to Object
 *   Store Manager.
 */
Error
ObjectStore::handleOnlineDiskFailures(DiskId& diskId, const diskio::DataTier& tier) {
    LOGDEBUG << "Handling disk failure for disk=" << diskId << " tier=" << tier;
    SmTokenSet lostTokens = diskMap->getSmTokens(diskId);
    if (g_fdsprocess->get_fds_config()->get<bool>("fds.sm.testing.useSsdForMeta")) {
        if (diskMap->getTotalDisks(tier) > 1) {
            diskMap->removeDiskAndRecompute(diskId, tier);
        } else {
            LOGCRITICAL << "Disk Failure. Node is out of disks!";
            return ERR_SM_NO_DISK;
        }
    } else {
        if (diskMap->getTotalDisks() > 1) {
            diskMap->removeDiskAndRecompute(diskId, tier);
        } else {
            LOGCRITICAL << "Disk Failure. Node is out of disks!";
            return ERR_SM_NO_DISK;
        }
    }
    Error err = openStore(lostTokens);
    if (!err.ok()) {
        LOGDEBUG << "Error opening metadata and data stores." << err;
    }

    if (requestResyncFn) {
        requestResyncFn();
    }
    return err;
}

/**
 * Open metadata and data store for given SM tokens
 * Ok if metadata and data stores are already open for any of
 * given tokens.
 */
Error
ObjectStore::openStore(const SmTokenSet& smTokens) {
    Error err(ERR_OK);
    // we may already have meta and data stores open for these
    // SM tokens, but call open in case some of the stores not open
    err = metaStore->openMetadataStore(diskMap, smTokens);
    if (!err.ok()) {
        LOGERROR << "Failed to open Metadata Store " << err;
        return err;
    }
    err = dataStore->openDataStore(diskMap, smTokens, false);
    if (!err.ok()) {
        LOGERROR << "Failed to open Data Store " << err;
    }
    return err;
}

Error
ObjectStore::handleNewDlt(const DLT* dlt) {
    fds_uint32_t nbits = dlt->getNumBitsForToken();
    metaStore->setNumBitsPerToken(nbits);

    // if Object store failed to initialize, will fail
    // validating token ownership
    ObjectStoreState curState = currentState.load();
    if (curState == OBJECT_STORE_UNAVAILABLE) {
        LOGCRITICAL << "Object Store unavailable, not updating token ownership";
        return ERR_NODE_NOT_ACTIVE;
    }

    Error err = diskMap->handleNewDlt(dlt);
    if (err == ERR_SM_NOERR_GAINED_SM_TOKENS) {
        // we gained new SM tokens -- open metadata store for these SM tokens
        Error openErr = metaStore->openMetadataStore(diskMap);
        if (!openErr.ok()) {
            LOGERROR << "Failed to open Metadata Store " << openErr;
            return openErr;
        }

        err = dataStore->openDataStore(diskMap, false);
    } else if (err == ERR_SM_NOERR_LOST_SM_TOKENS) {
        // disk map already verified that this happend on our first DLT
        // this is the case, where SM went down between DLT update and DLT close
        // and we did not reflect losing SM token ownership in superblock;
        // that's ok -- do it now by "simulating" DLT close
        LOGDEBUG << "Looks like we lost SM tokens, will invalidate";
        err = handleDltClose(dlt);

        // we are now always doing full resync on restart (see above comment, this
        // error happens only on first DLT update after restart
        if (err.ok()) {
            err = ERR_SM_NOERR_NEED_RESYNC;
        }
    } else if (err == ERR_SM_SUPERBLOCK_INCONSISTENT) {
        LOGCRITICAL << "We got DLT version that superblock already knows about ["
                    << dlt->getVersion() << "] but not all SM tokens are marked valid."
                    << " How do we handle this? Marking Object Store unavailable";
        err = ERR_PERSIST_STATE_MISMATCH;
        // second phase of initializing object store failed!
        currentState = OBJECT_STORE_UNAVAILABLE;
    } else if (err == ERR_SM_NOERR_NOT_IN_DLT) {
        // this is the case when SM started in pristine state, but restarted
        // before migration happened and it gained token ownership
        LOGDEBUG << "Looks like SM restarted before successfully joining the domain";
        // no resync needed, but set store ready so that it can do migration
        currentState = OBJECT_STORE_READY;
        err = ERR_OK;
    }

    // if updating disk map determined that this SM restart case and it succeeded
    // validating superblock, object store is ready to service IO and become a source
    // of migration. Note that at this point none of DLT tokens are not ready yet,
    // this is controlled by a data struct in the migration mgr
    if (err == ERR_SM_NOERR_NEED_RESYNC) {
        currentState = OBJECT_STORE_READY;
    }

    return err;
}

Error
ObjectStore::handleDltClose(const DLT* dlt) {
    Error err(ERR_OK);

    SmTokenSet rmTokens = diskMap->handleDltClose(dlt);
    if (rmTokens.size()) {
        // we lost SM tokens -- close & remove metadata store for these SM tokens
        err = metaStore->closeAndDeleteMetadataDbs(rmTokens);
        if (!err.ok()) {
            LOGERROR << "Failed to close Metadata DBs " << err;
            // ignoring for now -- nothing horrible should happen
        }

        // also close & remove data store
        err = dataStore->closeAndDeleteSmTokensStore(rmTokens);
        if (!err.ok()) {
            LOGERROR << "Failed to close token files " << err;
        }
    }

    return err;
}

Error
ObjectStore::addVolume(const VolumeDesc& volDesc) {
    Error err(ERR_OK);
    GLOGTRACE << "Adding volume " << volDesc;
    return err;
}

Error
ObjectStore::removeVolume(fds_volid_t volId) {
    Error err(ERR_OK);
    return err;
}

Error
ObjectStore::checkAvailability() const {
    ObjectStoreState curState = currentState.load();
    if (curState == OBJECT_STORE_UNAVAILABLE) {
        LOGERROR << "Object store in UNAVAILABLE state. "
                 << " It may not be initialized, or you may have run out of disk capacity.";
        return ERR_NODE_NOT_ACTIVE;
    } else if (curState == OBJECT_STORE_INIT) {
        LOGERROR << "Object Store is coming up, but not ready to accept IO";
        return ERR_NOT_READY;
    }
    return ERR_OK;
}

Error
ObjectStore::putObject(fds_volid_t volId,
                       const ObjectID &objId,
                       boost::shared_ptr<const std::string> objData,
                       fds_bool_t forwardedIO) {
    Error err = checkAvailability();
    if (!err.ok()) {
        return err;
    }

    fiu_return_on("sm.objectstore.faults.putObject", ERR_DISK_WRITE_FAILED);

    PerfContext objWaitCtx(PerfEventType::SM_PUT_OBJ_TASK_SYNC_WAIT, volId);

    PerfTracer::tracePointBegin(objWaitCtx);
    ScopedSynchronizer scopedLock(*taskSynchronizer, objId);
    PerfTracer::tracePointEnd(objWaitCtx);

    diskio::DataTier useTier = diskio::maxTier;
    LOGTRACE << "Putting object " << objId << " volume " << std::hex << volId
             << std::dec;

    // New object metadata to update the refcnts
    ObjMetaData::ptr updatedMeta;

    // INTERACTION WITH MIGRATION and ACTIVE IO (second phase of SM token migration)
    //
    // If the metadata exists at this point, following matrix of operation is possible:
    //
    //                                              Regular PUT or forwarded PUT
    //                                  |  ObjData physically exists  | ObjData doesn't exist |
    //              ---------------------------------------------------------------------------
    //                NO_RECONCILE      |               1             |          *2*          |
    //                or N/A            |                             |                       |
    // OnDiskFlag   ---------------------------------------------------------------------------
    //                RECONCILE         |               3             |           4           |
    //                (DELETE deficit)  |                             |                       |
    //              ---------------------------------------------------------------------------
    //
    // 1) Normal PUT operation
    //
    // 2) Cannot happen.  This condition is not possible with current operation.
    //
    // 3) Requires reconciliation.  The DISK(RECONCILE) and PUT must be reconciled, and end
    //    result may lead to condition 4 or condition 1.  Metadata update only.
    //
    // 4) Requires reconciliation.  Similar to condition 3, but requires metadata update +
    //    object write.
    //

    // Get metadata from metadata store
    ObjMetaData::const_ptr objMeta = metaStore->getObjectMetadata(volId, objId, err);
    if (err == ERR_OK) {

        // TokenMigration + Active IO: Condition 2).
        // This should never happen, so panic if this condition is hit.
        // If hit, there is a bug in token migration.
        if (!objMeta->isObjReconcileRequired()) {
            fds_verify(objMeta->dataPhysicallyExists());
        }

        // check if existing object corrupted
        if (objMeta->isObjCorrupted()) {
            LOGCRITICAL << "CORRUPTION: Dup object corruption detected: " << objMeta->logString()
                        << " returning err=" << ERR_SM_DUP_OBJECT_CORRUPT;
            return ERR_SM_DUP_OBJECT_CORRUPT;
        }

        if (conf_verify_data == true) {
            // verify data -- read object from object data store
            boost::shared_ptr<const std::string> existObjData
                    = dataStore->getObjectData(volId, objId, objMeta, err);
            if (!err.ok()) {
                return err;
            }
            // check if data is the same
            if (*existObjData != *objData) {
                fds_panic("Encountered a hash collision checking object %s. Bailing out now!",
                          objId.ToHex().c_str());
            }
        }  // if (conf_verify_data == true)

        // Create new object metadata to update the refcnts
        updatedMeta.reset(new ObjMetaData(objMeta));

        // TokenMigration + Active IO: Condition 3) and 4).
        // After reconciling, we have 4 possibilities:
        //      1) ONDISK(RECONCILE) -
        //          1.1) ObjData exists physically
        //          1.2) ObjData does not exit physically
        //      2) ONDISK(NO_RECONCILE)
        //          1.1) ObjData exists physically
        //          1.2) ObjData does not exit physically
        //
        // In any case, write out the metadata. And write out ObjData if data physically doesn't exist.
        if (updatedMeta->isObjReconcileRequired()) {
            updatedMeta->reconcilePutObjMetaData(objId, volId);
        }

        PerfTracer::incr(PerfEventType::SM_PUT_DUPLICATE_OBJ, volId);
        err = ERR_DUPLICATE;
    } else {  // if (getMetadata != OK)
        // We didn't find any metadata, make sure it was just not there and reset
        if (err == ERR_NOT_FOUND) {
            err = ERR_OK;
            updatedMeta.reset(new ObjMetaData());
            updatedMeta->initialize(objId, objData->size());
        } else {
            return err;
        }
    }
    fds_verify(err.ok() || (err == ERR_DUPLICATE));

    // If the TokenMigration reconcile is still required, then treat the object as not valid.
    if (!updatedMeta->isObjReconcileRequired()) {
        // either new data or dup, update assoc entry
        std::map<fds_volid_t, fds_uint64_t> vols_refcnt;
        updatedMeta->getVolsRefcnt(vols_refcnt);
        updatedMeta->updateAssocEntry(objId, volId);
        volumeTbl->updateDupObj(volId,
                                objId,
                                objData->size(),
                                true,
                                vols_refcnt);
    }

    // Put data in store if it's not a duplicate.
    // Or TokenMigration + Active IO handle:  if the ObjData doesn't physically exist, still write out
    // the obj data.
    //
    // We expect the data put to be atomic.
    // If we crash after the writing the data but before writing
    // the metadata, the orphaned object data will get cleaned up
    // on a subsequent scavenger pass.
    if (err.ok() ||
        ((err == ERR_DUPLICATE) && !objMeta->dataPhysicallyExists())) {
        // object not duplicate
        // select tier to put object
        StorMgrVolume *vol = volumeTbl->getVolume(volId);

        // Depending on when the IO is available on this SM and when the volume
        // information is propoated when the cluster restarts or SM service
        // restarts after offline.
        if (vol != NULL) {
            useTier = tierEngine->selectTier(objId, *vol->voldesc);
        } else {
            useTier = diskio::diskTier;
        }

        // Adjust the tier depending on the system disk topology.
        if (diskMap->getTotalDisks(useTier) == 0) {
            // there is no requested tier, use existing tier
            LOGDEBUG << "There is no " << useTier << " tier, will use existing tier";
            if (useTier == diskio::flashTier) {
                useTier = diskio::diskTier;
            } else if (useTier == diskio::diskTier) {
                useTier = diskio::flashTier;
            }
        }

        if (useTier == diskio::flashTier) {
            fds_bool_t ssdSuccess = diskMap->ssdTrackCapacityAdd(objId,
                    objData->size(), tierEngine->getFlashFullThreshold());

            if (!ssdSuccess) {
                LOGTRACE << "Exceeded SSD capacity, use disk tier";
                useTier = diskio::diskTier;
            }
        }

        if (diskMap->getTotalDisks(useTier) == 0) {
            LOGCRITICAL << "No disk capacity";
            return ERR_SM_EXCEEDED_DISK_CAPACITY;
        }

        // put object to datastore
        obj_phy_loc_t objPhyLoc;  // will be set by data store
        err = dataStore->putObjectData(volId, objId, useTier, objData, objPhyLoc);
        if (!err.ok()) {
            LOGERROR << "Failed to write " << objId << " to obj data store "
                     << err;
            if (useTier == diskio::flashTier) {
                diskMap->ssdTrackCapacityDelete(objId, objData->size());
            }
            return err;
        }

        // Notify tier engine of recent IO
        tierEngine->notifyIO(objId, FDS_SM_PUT_OBJECT, *vol->voldesc, useTier);

        // update physical location that we got from data store
        updatedMeta->updatePhysLocation(&objPhyLoc);
    }

    // write metadata to metadata store
    err = metaStore->putObjectMetadata(volId, objId, updatedMeta);

    return err;
}

boost::shared_ptr<const std::string>
ObjectStore::getObject(fds_volid_t volId,
                       const ObjectID &objId,
                       diskio::DataTier& usedTier,
                       Error& err) {
    err = checkAvailability();
    if (!err.ok()) {
        return nullptr;
    }

    PerfContext objWaitCtx(PerfEventType::SM_GET_OBJ_TASK_SYNC_WAIT, volId);
    PerfTracer::tracePointBegin(objWaitCtx);
    ScopedSynchronizer scopedLock(*taskSynchronizer, objId);
    PerfTracer::tracePointEnd(objWaitCtx);

    // INTERACTION WITH MIGRATION and ACTIVE IO (second phase of SM token migration)
    //
    // If the metadata exists when queried, following matrix of operations is possible:
    //
    //                                  |  ObjData physically exists  | ObjData doesn't exist |
    //              ---------------------------------------------------------------------------
    //                NO_RECONCILE      |              1              |          *2*          |
    //                or N/A            |                             |                       |
    // OnDiskFlag   ---------------------------------------------------------------------------
    //                RECONCILE         |              3              |           3           |
    //                (DELETE deficit)  |                             |                       |
    //              ---------------------------------------------------------------------------
    //
    // 1) normal get operation.
    // 2) cannot happen.  If DISK(NO_RECONCILE) is set, then migration reconciled issue, so
    //    the object must exist.  Otherwise, an MetaData entered DELECT deficit state and never
    //    recovered or reconciled.
    // 3) If DISK(RECONCILE) flag is set, then return an error.

    // Get metadata from metadata store
    ObjMetaData::const_ptr objMeta = metaStore->getObjectMetadata(volId, objId, err);
    if (!err.ok()) {
        LOGERROR << "Failed to get object metadata" << objId << " volume "
                 << std::hex << volId << std::dec << " " << err;
        return nullptr;
    }


    // check if object corrupted
    if (objMeta->isObjCorrupted()) {
        LOGCRITICAL << "CORRUPTION: On-disk data corruption detected: " << objMeta->logString()
                    << " returning err=" << ERR_ONDISK_DATA_CORRUPT;
        err = ERR_ONDISK_DATA_CORRUPT;
        return nullptr;
    }

    // Check if the object is reconciled properly.
    if (objMeta->isObjReconcileRequired()) {
        LOGNOTIFY << "Requires Reconciliation: " << objMeta->logString()
                  << " err=" << ERR_NOT_FOUND;
        err = ERR_NOT_FOUND;
        return nullptr;
    }

    /*
     * TODO(umesh): uncomment this when reference counting is used.
     *
    // If this Volume never put this object, then it should not access the object
    if (!objMeta->isVolumeAssociated(volId)) {
        err = ERR_NOT_FOUND;
        LOGWARN << "Volume " << std::hex << volId << std::dec << " aunauth access "
                << " to object " << objId << " returning " << err;
        return nullptr;
    }
    */

    // get object data
    boost::shared_ptr<const std::string> objData
            = dataStore->getObjectData(volId, objId, objMeta, err);
    if (!err.ok()) {
        LOGERROR << "Failed to get object data " << objId << " volume "
                 << std::hex << volId << std::dec << " " << err;
        return objData;
    }

    // return tier we read from
    usedTier = diskio::diskTier;
    if (objMeta->onFlashTier()) {
        usedTier = diskio::flashTier;
    }

    // verify data
    if (conf_verify_data) {
        ObjectID onDiskObjId;
        onDiskObjId = ObjIdGen::genObjectId(objData->c_str(),
                                            objData->size());
        if (onDiskObjId != objId) {
            fds_panic("Encountered a on-disk data corruption object %s \n != %s",
                      objId.ToHex().c_str(), onDiskObjId.ToHex().c_str());
        }
    }

    // We are passing max_tier here, if we decide we
    // care which tier GET is from we need to change this
    tierEngine->notifyIO(objId, FDS_SM_GET_OBJECT,
            *volumeTbl->getVolume(volId)->voldesc, diskio::maxTier);

    return objData;
}

boost::shared_ptr<const std::string>
ObjectStore::getObjectData(fds_volid_t volId,
                       const ObjectID &objId,
                       ObjMetaData::const_ptr objMetaData,
                       Error& err)
{
    err = checkAvailability();
    if (!err.ok()) {
        return nullptr;
    }

    PerfContext objWaitCtx(PerfEventType::SM_GET_OBJ_TASK_SYNC_WAIT, volId);
    PerfTracer::tracePointBegin(objWaitCtx);
    ScopedSynchronizer scopedLock(*taskSynchronizer, objId);
    PerfTracer::tracePointEnd(objWaitCtx);

    boost::shared_ptr<const std::string> objData
            = dataStore->getObjectData(volId, objId, objMetaData, err);
    if (!err.ok()) {
        LOGERROR << "Failed to get object data " << objId << " volume "
                 << std::hex << volId << std::dec << " " << err;
    }
    return objData;
}

Error
ObjectStore::deleteObject(fds_volid_t volId,
                          const ObjectID &objId,
                          fds_bool_t forwardedIO) {
    Error err = checkAvailability();
    if (!err.ok()) {
        return err;
    }

    PerfContext objWaitCtx(PerfEventType::SM_DELETE_OBJ_TASK_SYNC_WAIT, volId);
    PerfTracer::tracePointBegin(objWaitCtx);
    ScopedSynchronizer scopedLock(*taskSynchronizer, objId);
    PerfTracer::tracePointEnd(objWaitCtx);

    // New object metadata to update the refcnts
    ObjMetaData::ptr updatedMeta;

    // INTERACTION WITH MIGRATION and ACTIVE IO (second phase of SM token migration)
    //
    // If the metadata exists at this point, following matrix of operation is possible:
    //
    //                                          Normal IO                  Forwarded IO
    //                                          Mig not running  |         Mig running
    //                                  |  MetaData  | !MetaData |  MetaData  | !MetaData |
    //              -----------------------------------------------------------------------
    //                NO_RECONCILE      |      1     |     2     |     3      |           |
    //                or N/A            |            |           |            |     4     |
    // OnDiskFlag   -----------------------------------------------------------------------
    //                RECONCILE         |     *5*    |    n/a    |     6      |     n/a   |
    //                (DELETE deficit)  |   special  |           |            |           |
    //              -----------------------------------------------------------------------
    //
    // 1) Normal DELETE operation.
    //
    // 2) Normal failed DELETE operation.
    //
    // 3) Normal DELETE operation.
    //    For this case, it should never reach DELETE Deficit case.  DELETE Deficit can happen
    //    only if the MetaData is created with DELETE operation forwarded to the destination SM
    //    before the MetaData+ObjData migrated successfully to the destination SM.
    //
    // 4) The MetaData doesn't exist.  This will create a MetaData object with ONDISK(RECONCILE) flag.
    //
    // 5) This is an interesting case.  This can happen if multiple DELETE requests are forwarded on
    //    the same object (ObJX)  before ObjX is migrated.  However, if the refcnt on object X is N, but
    //    N+M DELETE requests are forwarded, we can get into this issue.
    //    For example DELETE(N+M) is forwarded and ObjX has refcnt P, where P is < (N+M).  Treat this
    //    as deleted object.
    //    In this case, GC should just reclaim this object.
    //
    // 6) Reconcile MetaData

    // Get metadata from metadata store
    ObjMetaData::const_ptr objMeta = metaStore->getObjectMetadata(volId, objId, err);
    if (!err.ok()) {
        fds_verify(err == ERR_NOT_FOUND);

        if (forwardedIO) {
            // Migration + Active IO: condition 4) -- forwarded DELETE and no MetaData.
            // Create a new metadata with ONDISK(RECONCILE) state.
            // There is no ObjData physically associated with this object.
            updatedMeta.reset(new ObjMetaData());
            updatedMeta->initializeDelReconcile(objId, volId);
            err = metaStore->putObjectMetadata(volId, objId, updatedMeta);
            if (err.ok()) {
                LOGMIGRATE << "Forwarded DELETE success.  Created an empty MetaData: "
                           << updatedMeta->logString();
                return ERR_OK;
            } else {
                LOGMIGRATE << "Forwarded DELETE failed to store: "
                           << updatedMeta->logString();
                return err;
            }
        } else {
            LOGDEBUG << "Not able to read existing object locations, "
                     << "assuming no prior entry existed " << objId;
            return ERR_OK;
        }
    }

    // if object corrupted, no point of updating metadata
    if (objMeta->isObjCorrupted()) {
        LOGCRITICAL << "CORRUPTION: On-disk data corruption detected: " << objMeta->logString()
                    << " returning err=" << ERR_ONDISK_DATA_CORRUPT;
        return err = ERR_ONDISK_DATA_CORRUPT;
    }

    // Create new object metadata to update the refcnts
    updatedMeta.reset(new ObjMetaData(objMeta));
    std::map<fds_volid_t, fds_uint64_t> vols_refcnt;
    updatedMeta->getVolsRefcnt(vols_refcnt);
    // remove volume assoc entry
    fds_bool_t change;
    // TODO(Sean):
    // This is a tricky part.  What happens if the metadata is forever RequireReconcile state?
    // The current approach is to have the GC clean it up, but is it the correct approach?
    if (updatedMeta->isObjReconcileRequired() && forwardedIO) {
        // Migration + Active IO: condition 6) -- forwarded DELETE and foundded MetaData require
        // reconciliation.
        // Since this is a DELETE operation, and the metadata is already require reconciliation,
        // this metadata will continue to to be "ReconcileRequired" (i.e. DELETE deficit).
        change = updatedMeta->reconcileDelObjMetaData(objId, volId, fds::util::getTimeStampMillis());
    } else {
        change = updatedMeta->deleteAssocEntry(objId, volId, fds::util::getTimeStampMillis());
    }

    if (!change) {
        // the volume was not associated, ok
        LOGNORMAL << "Volume " << std::hex << volId << std::dec
                  << " is not associated with this obj " << objId
                  << ", nothing to delete, returning OK";
        return ERR_OK;
    }

    // first write metadata to metadata store, even if removing
    // object from data store cache fails, it is ok
    err = metaStore->putObjectMetadata(volId, objId, updatedMeta);
    if (err.ok()) {
        PerfTracer::incr(PerfEventType::SM_OBJ_MARK_DELETED, volId);
        volumeTbl->updateDupObj(volId,
                                objId,
                                updatedMeta->getObjSize(),
                                false,
                                vols_refcnt);
        LOGDEBUG << "Decremented refcnt for object " << objId
                 << " " << *updatedMeta << " refcnt: "
                 << updatedMeta->getRefCnt();
    } else {
        LOGERROR << "Failed to update metadata for object " << objId
                 << " " << err;
        return err;
    }

    // if refcnt is 0, notify data store so it can remove cache entry, etc
    if (updatedMeta->getRefCnt() < 1L) {
        err = dataStore->removeObjectData(volId, objId, updatedMeta);
    }

    // notifying tier engine, change this if we decide we care which tier the delete happens from
    tierEngine->notifyIO(objId, FDS_SM_DELETE_OBJECT,
            *volumeTbl->getVolume(volId)->voldesc, diskio::maxTier);

    return err;
}

Error
ObjectStore::moveObjectToTier(const ObjectID& objId,
                              diskio::DataTier fromTier,
                              diskio::DataTier toTier,
                              fds_bool_t relocateFlag) {
    Error err(ERR_OK);
    ScopedSynchronizer scopedLock(*taskSynchronizer, objId);

    LOGDEBUG << "Moving object " << objId << " from tier " << fromTier
             << " to tier " << toTier << " relocate?" << relocateFlag;

    // since object can be associated with multiple volumes, we just
    // set volume id to invalid. Here is it used only to collect perf
    // stats and associate them with a volume; moving objects between
    // tiers will be accounted against volume 0
    fds_volid_t unknownVolId = invalid_vol_id;

    // Get metadata from metadata store
    ObjMetaData::const_ptr objMeta = metaStore->getObjectMetadata(unknownVolId, objId, err);
    if (!err.ok()) {
        LOGERROR << "Failed to get metadata for object " << objId << " " << err;
        return err;
    }

    // do not move if object is deleted
    if (objMeta->getRefCnt() < 1) {
        LOGWARN << "object " << objId << " refcnt == 0";
        return ERR_SM_ZERO_REFCNT_OBJECT;
    }

    if (relocateFlag &&
        fromTier == diskio::DataTier::flashTier &&
        toTier == diskio::DataTier::diskTier) {
        std::vector<fds_volid_t> vols;
        objMeta->getAssociatedVolumes(vols);
        /* Skip moving from flash to disk if all associated volumes are flash only */
        if (!(volumeTbl->hasFlashOnlyVolumes(vols))) {
            /* NOTE: Phyically moving the object should be taken care by GC */
            return updateLocationFromFlashToDisk(objId, objMeta);
        } else {
            return ERR_SM_TIER_HYBRIDMOVE_ON_FLASH_VOLUME;
        }
    }

    // make sure the object is not on destination tier already
    if (objMeta->onTier(toTier)) {
        LOGERROR << "Object " << objId << " is already on tier " << toTier;
        return ERR_DUPLICATE;
    }

    // read object from fromTier
    boost::shared_ptr<const std::string> objData
            = dataStore->getObjectData(unknownVolId, objId, objMeta, err);
    if (!err.ok()) {
        LOGERROR << "Failed to get object data " << objId << " for copying "
                 << "to tier " << toTier << " " << err;
        return err;
    }

    // write to object data store to toTier
    obj_phy_loc_t objPhyLoc;  // will be set by data store with new location
    err = dataStore->putObjectData(unknownVolId, objId, toTier, objData, objPhyLoc);
    if (!err.ok()) {
        LOGERROR << "Failed to write " << objId << " to obj data store "
                 << ", tier " << toTier << " " << err;
        return err;
    } // update physical location that we got from data store
    ObjMetaData::ptr updatedMeta(new ObjMetaData(objMeta));
    updatedMeta->updatePhysLocation(&objPhyLoc);
    if (relocateFlag) {
        // remove from fromTier
        updatedMeta->removePhyLocation(fromTier);
    }

    // write metadata to metadata store
    err = metaStore->putObjectMetadata(unknownVolId, objId, updatedMeta);
    if (!err.ok()) {
        LOGERROR << "Failed to update metadata for obj " << objId;
    }
    return err;
}

Error ObjectStore::updateLocationFromFlashToDisk(const ObjectID& objId,
                                                 ObjMetaData::const_ptr objMeta)
{
    Error err;

    if (!objMeta->onTier(diskio::DataTier::diskTier)) {
        LOGNOTIFY << "Object " << objId << " hasn't made it to disk yet.  Ignoring move";
        return ERR_SM_TIER_WRITEBACK_NOT_DONE;
    }

    /* Remove flash as the phsycal location */
    ObjMetaData::ptr updatedMeta(new ObjMetaData(objMeta));
    updatedMeta->removePhyLocation(diskio::DataTier::flashTier);
    fds_assert(updatedMeta->onTier(diskio::DataTier::diskTier) == true);
    fds_assert(updatedMeta->onTier(diskio::DataTier::flashTier) == false);

    /* write metadata to metadata store */
    err = metaStore->putObjectMetadata(invalid_vol_id, objId, updatedMeta);
    if (!err.ok()) {
        LOGERROR << "Failed to update metadata for obj " << objId;
    } else {
        // TODO(Rao): Remove this log statement
        DBG(LOGDEBUG << "Moved object " << objId << "from flash to disk");
    }
    return err;
}

Error
ObjectStore::copyAssociation(fds_volid_t srcVolId,
                             fds_volid_t destVolId,
                             const ObjectID& objId) {
    Error err = checkAvailability();
    if (!err.ok()) {
        return err;
    }

    PerfContext objWaitCtx(PerfEventType::SM_ADD_OBJ_REF_TASK_SYNC_WAIT, destVolId);
    PerfTracer::tracePointBegin(objWaitCtx);
    ScopedSynchronizer scopedLock(*taskSynchronizer, objId);
    PerfTracer::tracePointEnd(objWaitCtx);

    // New object metadata to update association
    ObjMetaData::ptr updatedMeta;

    // Get metadata from metadata store
    ObjMetaData::const_ptr objMeta = metaStore->getObjectMetadata(srcVolId, objId, err);
    if (!err.ok()) {
        LOGERROR << "Failed to get metadata for object " << objId << " " << err;
        return err;
    }

    // TODO(Sean):
    // Here, we have to careful about what it means to have forwarded copyAssociation()
    // operation (i.e. AddObjectRef()) and not have the objMetaData not present.
    // We will have to handle it similar to other data paths (i.e. PUT and DELETE), but
    // not sure if doing it at this point makes too much sense, since DM side of
    // it will be re-done.  For now, make a note and re-visit, once DM has the
    // design.

    // copy association entry
    updatedMeta.reset(new ObjMetaData(objMeta));

    // TODO(Sean):
    // Why do we need to get volsrefcnt here?  What do we do with it?
    std::map<fds_volid_t, fds_uint64_t> vols_refcnt;
    updatedMeta->getVolsRefcnt(vols_refcnt);
    updatedMeta->copyAssocEntry(objId, srcVolId, destVolId);

    // write updated metadata to meta store
    err = metaStore->putObjectMetadata(destVolId, objId, updatedMeta);
    if (err.ok()) {
        // TODO(xxx) we should call updateDupObj()
    } else {
        LOGERROR << "Failed to add association entry for object " << objId
                 << " to object metadata store " << err;
    }

    return err;
}

Error
ObjectStore::copyObjectToNewLocation(const ObjectID& objId,
                                     diskio::DataTier tier,
                                     fds_bool_t verifyData,
                                     fds_bool_t objOwned) {
    ScopedSynchronizer scopedLock(*taskSynchronizer, objId);
    Error err(ERR_OK);

    // since object can be associated with multiple volumes, we just
    // set volume id to invalid. Here is it used only to collect perf
    // stats and associate them with a volume; in case of GC, all metadata
    // and disk accesses will be counted against volume 0
    fds_volid_t unknownVolId = invalid_vol_id;

    // Get metadata from metadata store
    ObjMetaData::const_ptr objMeta = metaStore->getObjectMetadata(unknownVolId, objId, err);
    if (!err.ok()) {
        LOGERROR << "Failed to get metadata for object " << objId << " " << err;
        return err;
    }

    if (!TokenCompactor::isDataGarbage(*objMeta, tier) &&
        objOwned) {
        // this object is valid, copy it to a current token file
        ObjMetaData::ptr updatedMeta;
        LOGDEBUG << "Will copy " << objId << " to new file on tier " << tier;

        // first read the object
        boost::shared_ptr<const std::string> objData
                = dataStore->getObjectData(unknownVolId, objId, objMeta, err);
        if (!err.ok()) {
            LOGERROR << "Failed to get object data " << objId << " for copying "
                     << "to new file (not garbage collect) " << err;
            return err;
        }
        // Create new object metadata for update
        updatedMeta.reset(new ObjMetaData(objMeta));

        // we may be copying file with objects that already has 'corrupt'
        // flag set. Since we are not yet recovering corrupted objects, we
        // are going to copy corrupted objects to new files (not loose them)
        if (!objMeta->isObjCorrupted() && verifyData) {
            ObjectID onDiskObjId;
            onDiskObjId = ObjIdGen::genObjectId(objData->c_str(),
                                                objData->size());
            if (onDiskObjId != objId) {
                // on-disk data corruption
                // mark object metadata as corrupted! will copy it to new
                // location anyway so we can debug the issue
                LOGCRITICAL << "CORRUPTION: On-disk corruption detected: "
                            << objId.ToHex() << "!=" <<  onDiskObjId.ToHex()
                            << " ObjMetaData=" << updatedMeta->logString();
                // set flag in object metadata
                updatedMeta->setObjCorrupted();
            }
        }

        // write to object data store (will automatically write to new file)
        obj_phy_loc_t objPhyLoc;  // will be set by data store with new location
        err = dataStore->putObjectData(unknownVolId, objId, tier, objData, objPhyLoc);
        if (!err.ok()) {
            LOGERROR << "Failed to write " << objId << " to obj data store "
                     << ", tier " << tier << " " << err;
            return err;
        }

        // update physical location that we got from data store
        updatedMeta->updatePhysLocation(&objPhyLoc);
        // write metadata to metadata store
        err = metaStore->putObjectMetadata(unknownVolId, objId, updatedMeta);
        if (!err.ok()) {
            LOGERROR << "Failed to update metadata for obj " << objId;
        }
    } else {
        // not going to copy object to new location
        LOGDEBUG << "Will garbage-collect " << objId << " on tier " << tier;
        // remove entry from index db if data + meta is garbage
        if (TokenCompactor::isGarbage(*objMeta) || !objOwned) {
            LOGDEBUG << "Removing metadata for " << objId
                      << " object owned? " << objOwned;
            err = metaStore->removeObjectMetadata(unknownVolId, objId);
        }
    }

    return err;
}

Error
ObjectStore::applyObjectMetadataData(const ObjectID& objId,
                                     const fpi::CtrlObjectMetaDataPropagate& msg) {

    Error err = checkAvailability();
    if (!err.ok()) {
        return err;
    }

    // we do not expect to receive rebal message for same object id concurrently
    // but we may do GC later while migrating, etc, so locking anyway
    ScopedSynchronizer scopedLock(*taskSynchronizer, objId);

    bool isDataPhysicallyExist = false;
    bool metadataAlreadyReconciled = false;

    diskio::DataTier useTier = diskio::maxTier;
    // since object can be associated with multiple volumes, we just
    // set volume id to invalid. Here is it used only to collect perf
    // stats and associate them with a volume; all metadata and disk
    // accesses for token migration will be counted against volume 0
    fds_volid_t unknownVolId = invalid_vol_id;

    // We will update metadata with metadata sent to us from source SM
    ObjMetaData::ptr updatedMeta;

    LOGMIGRATE << "Applyying Object: " << fds::logString(msg);

    // INTERACTION WITH MIGRATION and ACTIVE IO (second phase of SM token migration)
    //
    // If the metadata exists at this point, following matrix of operation is possible:
    //
    //                                          MetadataPropagateMsg
    //                               |  NO_RECONCILE |  RECONCILE   | OVERWRITE    |
    //                               |  (equiv PUT)  |  (meta only) | (meta only)  |
    //             -----------------------------------------------------------------
    //             NO_RECONCILE      |               |              |              |
    //             or N/A            |      1        |      2       |      *3*     |
    // OnDiskFlag  -----------------------------------------------------------------
    //             RECONCILE         |      4        |     *5*      |      *6*     |
    //             (DELETE deficit)  |               |              |              |
    //             -----------------------------------------------------------------
    //
    // 1) Object was forwarded, and a new object in the second snapshot is migrated.  Same ObjData and
    //    requires aggregation/reconcile metadata.
    //
    // 2) On disk metadata and MetadataPropagateMsg metadat needs reconciliation.  Typical case.
    //
    // 3) This applies only to the first phase, where metadata is overwritten
    //    if ObjectData already exists on the destination.
    //
    // 4) New object in the second snapshot from the Client SM.  However, at least one DELETE
    //    operation was forwarded to the object, or the net result of forwarded DELETE and PUT yielded
    //    DELETE deficit.  Need to reconcile MetaData and write out ObjectData if not data exists.
    //
    // 5) This cannot happen.  MSG(RECONCILE) in msg means that the Object was in both the first and
    //    second snapshot.  ONDISK(RECONCILE) (i.e. DELETE deficit) means that the MetaData
    //    operation was forwarded before a new Object in the second snapshot was migrated.
    //    So, this cannot happen.
    //
    // 6) This cannot happen.  MSG(OVERWRITE) only happens on the first snapshot.

    // Get metadata from metadata store to check if there is an existin metadata
    ObjMetaData::const_ptr objMeta = metaStore->getObjectMetadata(unknownVolId, objId, err);
    if (err == ERR_OK) {

        // check if existing object corrupted
        if (objMeta->isObjCorrupted()) {
            LOGCRITICAL << "CORRUPTION: Dup object corruption detected: " << objMeta->logString()
                        << " returning err=" << ERR_SM_DUP_OBJECT_CORRUPT;
            return ERR_SM_DUP_OBJECT_CORRUPT;
        }

        // if we got data with this message, check if it matches data stored on this SM
        if ((msg.objectData.size() != 0) &&
            (conf_verify_data == true)) {
            // verify data -- read object from object data store
            // data in this msg
            boost::shared_ptr<const std::string> objData = boost::make_shared<std::string>(
                msg.objectData);
            // data stored in object store
            boost::shared_ptr<const std::string> existObjData
                    = dataStore->getObjectData(unknownVolId, objId, objMeta, err);
            // if we get an error, there are inconsistencies between
            // data and metadata; assert for now
            fds_assert(err.ok());

            // check if data is the same
            if (*existObjData != *objData) {
                LOGCRITICAL << "CORRUPTION: Mismatch between data in object store and data received "
                            << "from source SM for " << objId << " !!!";
                return ERR_SM_TOK_MIGRATION_DATA_MISMATCH;
            }
        }

        // create new object metadata for update from source SM
        updatedMeta.reset(new ObjMetaData(objMeta));

        isDataPhysicallyExist = updatedMeta->dataPhysicallyExists();

        // we temporary assign error duplicate to indicate that we already have metadata
        // and data for this object, to differentiate from the case when this is the
        // first time this SM sees this object
        err = ERR_DUPLICATE;

        // Migration + Active IO:  condition 1) -- a case where MSG(NO_RECONCILE) and DISK(NO_RECONCILE).
        //            This happens when a PUT IO was forwarded for a new object, in the second snapshot,
        //            had a chance to migrate to the destination SM.
        //            One possible example: PUT was forwarded for ObjX.  ObjX is migrated from
        //            source SM to destination SM.
        if (msg.objectReconcileFlag == fpi::OBJ_METADATA_NO_RECONCILE) {

            metadataAlreadyReconciled = true;

            LOGMIGRATE << "MSG(NO_RECONCILE): "
                       << "Before PropagateMsg conversion: " << logString(msg);

            updatedMeta->reconcileDeltaObjMetaData(msg);

            LOGMIGRATE << "MSG(NO_RECONCILE): "
                       << "After PropagateMsg conversion: " << logString(msg);
            fds_verify(err == ERR_DUPLICATE);
        }

    } else {  // if (getMetadata != OK)
        // On the second phase, if the metadata is not found, then it's a new Object.
        // can just write out object.

        // We didn't find any metadata, make sure it was just not there and reset
        // This is a new object.  There should not be any reconciliation.
        fds_verify(err == ERR_NOT_FOUND);

        // If we didn't find the metadata on the destination SM, then there is
        // no need for metadata reconcile or overwrite
        fds_assert(msg.objectReconcileFlag == fpi::OBJ_METADATA_NO_RECONCILE);

        err = ERR_OK;

        isDataPhysicallyExist = false;

        // make sure we got object data as well in this message
        if (msg.objectData.size() == 0) {
            LOGERROR << "Got zero-length objectData for object " << objId
                     << " in rebalance delta set from source SM, while this SM "
                     << " does not have metadata/data for this object";
            return ERR_SM_TOK_MIGRATION_NO_DATA_RECVD;
        }
        updatedMeta.reset(new ObjMetaData());
        updatedMeta->initialize(objId, msg.objectData.size());
    }

    // Put data in store if we got data/metadata. We expect the data put to be atomic.
    // If we crash after the writing the data but before writing
    // the metadata, the orphaned object data will get cleaned up
    // on a subsequent scavenger pass.
    if (false == isDataPhysicallyExist) {

        // new object in this SM, put object to data store
        boost::shared_ptr<const std::string> objData = boost::make_shared<std::string>(
            msg.objectData);

        // we have to select tier based on volume policy with the highest tier policy
        StorMgrVolume* selectVol = NULL;
        for (auto volAssoc : msg.objectVolumeAssoc) {
            fds_volid_t volId(volAssoc.volumeAssoc);
            StorMgrVolume* vol = volumeTbl->getVolume(volId);
            //
            // TODO(Sean):  Volume table should be updated before token resync.
            //
            // At this point, SM may not have all volume information.  This can be
            // called during the SM Restart Resync path, which may or may not have the
            // latest volume information.  Therefore, volume associated with this
            // propagate message's ObjectMetaData may not exist yet.
            // Continue to look for a valid volume information, but if not found, use
            // tier::HDD.
            if (vol == NULL) {
                continue;
            }
            if (vol->voldesc->mediaPolicy == fpi::FDSP_MEDIA_POLICY_SSD) {
                selectVol = vol;
                break;   // ssd-only is highest media policy
            } else if (vol->voldesc->mediaPolicy == fpi::FDSP_MEDIA_POLICY_HYBRID) {
                // we didn't find ssd-only volume yet, so potential candidate
                // but we may see ssd-only volumes, so continue search
                selectVol = vol;
            } else if (vol->voldesc->mediaPolicy == fpi::FDSP_MEDIA_POLICY_HYBRID_PREFCAP) {
                if (selectVol->voldesc->mediaPolicy != fpi::FDSP_MEDIA_POLICY_HYBRID) {
                    selectVol = vol;
                }
            } else if (!selectVol) {
                selectVol = vol;
            }
        }

        // select tier to put object
        if (selectVol != NULL) {
            // If the volume is selected, then use the tier policy associated with
            // the volume.
            useTier = tierEngine->selectTier(objId, *selectVol->voldesc);
        } else {
            // If the volume doesn't exist, then use HDD tier as default.
            // If the system is all SSD, then the next block of code will re-adjust.
            useTier = diskio::diskTier;
        }

        // Adjust the tier depending on the system disk topolgy
        if (diskMap->getTotalDisks(useTier) == 0) {
            // there is no requested tier, use existing tier
            LOGDEBUG << "There is no " << useTier << " tier, will use existing tier";
            if (useTier == diskio::flashTier) {
                useTier = diskio::diskTier;
            } else if (useTier == diskio::diskTier) {
                useTier = diskio::flashTier;
            }
        }

        if (useTier == diskio::flashTier) {
            fds_bool_t ssdSuccess = diskMap->ssdTrackCapacityAdd(objId,
                    objData->size(), tierEngine->getFlashFullThreshold());

            if (!ssdSuccess) {
                useTier = diskio::diskTier;
            }
        }

        if (diskMap->getTotalDisks(useTier) == 0) {
            LOGCRITICAL << "No disk capacity";
            return ERR_SM_EXCEEDED_DISK_CAPACITY;
        }

        // put object to datastore
        obj_phy_loc_t objPhyLoc;  // will be set by data store
        err = dataStore->putObjectData(unknownVolId, objId, useTier, objData, objPhyLoc);
        if (!err.ok()) {
            LOGERROR << "Failed to write " << objId << " to obj data store "
                     << err;
            if (useTier == diskio::flashTier) {
                diskMap->ssdTrackCapacityDelete(objId, objData->size());
            }
            return err;
        }


        // Notify tier engine of recent IO if the volume information is available.
        if (NULL != selectVol) {
            tierEngine->notifyIO(objId, FDS_SM_PUT_OBJECT, *selectVol->voldesc, useTier);
        }

        // update physical location that we got from data store
        updatedMeta->updatePhysLocation(&objPhyLoc);
    }


    // TODO(Sean):
    // So, if the reconcile flag is OVERWRITE, how would it work with tiering?  Overwrite
    // means that a SM node is added with existing data.  Not sure what it means to added
    // a new node with existing objects and volume.  What if the volume policy has changed?
    // How does volume policy propagated to SM?
    // For now, do nothing with tiering, and keep the existing tier.

    // update metadata
    // note that we are not updating assoc entry as with datapath put
    // we are copying assoc entries from the msg and also copying ref count

    if (false == metadataAlreadyReconciled) {
        err = updatedMeta->updateFromRebalanceDelta(msg);
        if (!err.ok()) {
            LOGCRITICAL << "Failed to update metadata from delta from source SM " << err;
        }
    }

    // write metadata to metadata store
    if (err.ok()) {
        err = metaStore->putObjectMetadata(unknownVolId, objId, updatedMeta);
    }

    LOGDEBUG << "Applied object data/metadata to object store " << objId
             << ": Delta from src SM " << fds::logString(msg)
             << " >>> "
             << " Updated meta " << *updatedMeta << " "
             << " Error=" << err;
    return err;
}

void
ObjectStore::snapshotMetadata(fds_token_id smTokId,
                              SmIoSnapshotObjectDB::CbType notifFn,
                              SmIoSnapshotObjectDB* snapReq) {
    metaStore->snapshot(smTokId, notifFn, snapReq);
}

void
ObjectStore::snapshotMetadata(fds_token_id smTokId,
                              SmIoSnapshotObjectDB::CbTypePersist notifFn,
                              SmIoSnapshotObjectDB* snapReq) {
    metaStore->snapshot(smTokId, notifFn, snapReq);
}

Error
ObjectStore::scavengerControlCmd(SmScavengerCmd* scavCmd) {
    return dataStore->scavengerControlCmd(scavCmd);
}

Error
ObjectStore::tieringControlCmd(SmTieringCmd* tierCmd) {
    Error err(ERR_OK);
    LOGDEBUG << "Executing tiering command " << tierCmd->command;
    switch (tierCmd->command) {
        case SmTieringCmd::TIERING_ENABLE:
            tierEngine->enableTierMigration();
            break;
        case SmTieringCmd::TIERING_DISABLE:
            tierEngine->disableTierMigration();
            break;
        case SmTieringCmd::TIERING_START_HYBRIDCTRLR_MANUAL:
            tierEngine->startHybridTierCtrlr(true);
            break;
        case SmTieringCmd::TIERING_START_HYBRIDCTRLR_AUTO:
            tierEngine->startHybridTierCtrlr(false);
            break;
        default:
            fds_panic("Unknown tiering command");
    }
    return err;
}

void
ObjectStore::SmCheckUpdateDLT(const DLT *latestDLT)
{
    SMCheckCtrl->updateSMCheckDLT(latestDLT);
}
Error
ObjectStore::SmCheckControlCmd(SmCheckCmd *checkCmd)
{
    Error err(ERR_OK);

    LOGDEBUG << "Executing SM Check command " << checkCmd->command;
    switch (checkCmd->command) {
        case SmCheckCmd::SMCHECK_START:
            err = SMCheckCtrl->startSMCheck(static_cast<SmCheckActionCmd*>(checkCmd)->tgtTokens);
            break;
        case SmCheckCmd::SMCHECK_STOP:
            err = SMCheckCtrl->stopSMCheck();
            break;
        case SmCheckCmd::SMCHECK_STATUS:
            {
                SmCheckStatusCmd *statusCmd = static_cast<SmCheckStatusCmd *>(checkCmd);
                err = SMCheckCtrl->getStatus(statusCmd->checkStatus);
            }
            break;
        default:
            fds_panic("Unknown SM Check command");
    }
    return err;
}

fds_uint32_t
ObjectStore::getDiskCount() const {
    return diskMap->getTotalDisks();
}

void
ObjectStore::updateMediaTrackers(fds_token_id smTokId,
                                 diskio::DataTier tier,
                                 const Error& error) {
    if ((error == ERR_DISK_WRITE_FAILED) ||
        (error == ERR_DISK_READ_FAILED) ||
        (error == ERR_NO_BYTES_READ)) {
        DiskId diskId = diskMap->getDiskId(smTokId, tier);
        LOGDEBUG << "Feeding IO failed event for disk " << diskId
                 << " due to error " << error;
        if (tier == diskio::diskTier) {
            objStoreDiskMediaTracker.feed_event(ERR_DISK_WRITE_FAILED,
                                                diskId);
        } else if (tier == diskio::flashTier) {
            objStoreFlashMediaTracker.feed_event(ERR_DISK_WRITE_FAILED,
                                                 diskId);
        }
    }
}

/**
 * Handle disk removal from the system.
 * For Hybrid Storage System(2SSD - 10HDD default config):
 *                    Metadata is stored on SSDs and data on HDDs.
 * For All SSD system:
 *                    Metadata and data are stored on the same SSD.
 *
 * If HDD is removed: Delete the corresponding metadata levelDBs
 *                    for the smTokens whose token files are lost
 *                    due to disk removal.
 * If SSD is removed: For hybrid system, remove token files of lost
 *                    smTokens.
 *                    For all SSD system, since data and metadata
 *                    was stored on the same disk. Nothing to be
 *                    done.
 */
void
ObjectStore::handleDiskChanges(const diskio::DataTier& tierType,
                               const TokenDiskIdPairSet& tokenDiskPairs) {
    if (!diskMap->isAllDisksSSD()) {
        switch (tierType) {
            case diskio::diskTier:
                if (g_fdsprocess->get_fds_config()->get<bool>("fds.sm.testing.useSsdForMeta")) {
                    LOGNOTIFY << "Close and delete metadata DBs for smTokens ";
                    /**
                     * Delete persisted levelDB for given SM Tokens.
                     */
                    for (auto& tokenPair: tokenDiskPairs) {
                        LOGNOTIFY << tokenPair.first;
                        metaStore->closeAndDeleteMetadataDb(tokenPair.first);
                        //metaStore->deleteMetadataDb(diskMap->getDiskPath(tokenPair.second),
                        //                            tokenPair.first);
                    }
                }
                break;
            case diskio::flashTier:
                LOGNOTIFY << "Close and delete token files for smTokens ";
                for (auto& tokenPair: tokenDiskPairs) {
                    LOGNOTIFY << tokenPair.first;
                    dataStore->deleteObjectDataFile(diskMap->getDiskPath(tokenPair.second),
                                                    tokenPair.first, tokenPair.second);
                }
                break;
            default:
                fds_panic("Unidentified disk type");
                LOGWARN << "Unidentified disk type removed. No disk failure handling done";
        }
    }
}

/**
 * Module initialization
 */
int
ObjectStore::mod_init(SysParams const *const p) {
    static Module *objStoreDepMods[] = {
        tierEngine.get(),
        dataStore.get(),
        metaStore.get(),
        diskMap.get(),
        NULL
    };
    mod_intern = objStoreDepMods;
    Module::mod_init(p);

    initObjectStoreMediaErrorHandlers();
    // Conditionally enable write faults at the given rate
    float write_failure_rate = g_fdsprocess->get_fds_config()->get<float>("fds.sm.objectstore.faults.fail_writes", 0.0f);
    if (write_failure_rate != 0.0f) {
        fiu_enable_random("sm.objectstore.faults.putObject", 1, nullptr, 0, write_failure_rate);
    }

    conf_verify_data = g_fdsprocess->get_fds_config()->get<bool>("fds.sm.data_verify");
    taskSyncSize =
            g_fdsprocess->get_fds_config()->get<fds_uint32_t>(
                "fds.sm.objectstore.synchronizer_size");
    taskSynchronizer = std::unique_ptr<HashedLocks<ObjectID, ObjectHash>>(
        new HashedLocks<ObjectID, ObjectHash>(taskSyncSize));

    // check if there is at least one device
    if (diskMap->getTotalDisks() == 0) {
        LOGCRITICAL << "Object Store is unavailable, until at least one device is added";
        currentState = OBJECT_STORE_UNAVAILABLE;
        return 0;
    }

    // do initial validation of SM persistent state
    Error err = diskMap->loadPersistentState();
    fiu_do_on("sm.objectstore.faults.init.firstphase", err = ERR_SM_SUPERBLOCK_NO_RECONCILE; );
    if (err.ok() || (err == ERR_SM_NOERR_PRISTINE_STATE)) {
        // open metadata store for tokens owned by this SM
        Error openErr = metaStore->openMetadataStore(diskMap);
        if (!openErr.ok()) {
            LOGERROR << "Failed to open Metadata Store " << openErr;
            return -1;
        } else {
            // open data store for tokens owned by this SM
            openErr = dataStore->openDataStore(diskMap,
                                               (err == ERR_SM_NOERR_PRISTINE_STATE));
        }
        LOGDEBUG << "First phase of object store init done";

        // if object store comes up in pristine state, then we are done
        // initializing it -- set state to ready
        if (err == ERR_SM_NOERR_PRISTINE_STATE) {
            currentState = OBJECT_STORE_READY;
        }
    } else {
        LOGCRITICAL << "Object Store failed to initialize! " << err;
        currentState = OBJECT_STORE_UNAVAILABLE;
    }

    return 0;
}

/**
 * Module startup
 */
void
ObjectStore::mod_startup() {
    Module::mod_startup();
}

/**
 * Module shutdown
 */
void
ObjectStore::mod_shutdown() {
    Module::mod_shutdown();
}

}  // namespace fds
