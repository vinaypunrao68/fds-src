/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTSTORE_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTSTORE_H_

#include <string>
#include <fds_module.h>
#include <fds_volume.h>
#include <StorMgrVolumes.h>
#include <concurrency/HashedLocks.hpp>
#include <object-store/SmDiskMap.h>
#include <TierEngine.h>
#include <object-store/ObjectStoreCommon.h>
#include <object-store/ObjectDataStore.h>
#include <object-store/ObjectMetadataStore.h>
#include <persistent-layer/dm_io.h>
#include <utility>
#include <SMCheckCtrl.h>
#include <util/EventTracker.h>

namespace fds {

typedef std::function <void(fds_bool_t, fds_bool_t)> StartResyncFnObj;
typedef std::function <void(SmTokenSet&)> TokenOfflineFnObj;

typedef std::set<std::pair<fds_token_id, fds_uint16_t>> TokenDiskIdPairSet;

/**
 * The ObjectStore manages persistent storage of Formation Objects, which
 * are content addressable key-value pairs. The ObjectStore provides
 * a simple interface to put, get, and delete these pairs per volume. Since
 * the objects are content addressable, put and delete simply update reference
 * counts. The storage is provided at a per-volume level and managed by
 * the object store.
 */
class ObjectStore : public Module, public boost::noncopyable {
  private:
    /// Object data storage
    ObjectDataStore::unique_ptr dataStore;

    /// Object metadata storage
    ObjectMetadataStore::unique_ptr metaStore;

    /// SM disk map keeps track of SM tokens and their locations
    SmDiskMap::ptr diskMap;

    /// Tiering engine
    TierEngine::unique_ptr tierEngine;

    /// SM Checker
    SMCheckControl::unique_ptr SMCheckCtrl;

    enum ObjectStoreState {
        /**
         * The object store is in initializing state before it
         * validated its superblock and checked token ownership
         */
        OBJECT_STORE_INIT        = 0,
        /**
         * The object store is in ready state if it successfully
         * loaded and validated superblock, minimal data integrity
         * check succeeded, and token ownership state in superblock
         * is successfully verified and consistent
         * In this state, object store is also ready to become a source
         * of token migration
         */
        OBJECT_STORE_READY       = 1,
        /**
         * Something bad happend (e.g., minimal data integrity check failed)
         * so the object store is currently un-usable
         */
        OBJECT_STORE_UNAVAILABLE = 2,
        OBJECT_STORE_STATE_MAX
    };

    /// Current state of the object store
    std::atomic<ObjectStoreState> currentState;

    /// Volume table for accessing vol descriptors
    // Does not own, passed from SM processing layer
    // TODO(xxx) we should be able to get this from platform
    StorMgrVolumeTable *volumeTbl;

    /// Resync request to Object store manager to start migration.
    StartResyncFnObj requestResyncFn;

    /// Callback to request Migration Manager(via Object Store Mgr)
    /// to make given sm tokens offline.
    TokenOfflineFnObj changeTokensStateFn;

    /// config params
    fds_bool_t conf_verify_data;

    /// Task synchronizer
    std::unique_ptr<HashedLocks<ObjectID, ObjectHash>> taskSynchronizer;
    /// Size of the synchronizer (controls false positives)
    fds_uint32_t taskSyncSize;
    typedef ScopedHashedLock<ObjectID,
                             HashedLocks<ObjectID, ObjectHash>> ScopedSynchronizer;

    // to track disk errors: disk id -> error
    typedef EventTracker<fds_uint16_t, Error,
                         std::hash<fds_uint16_t>,
                         ErrorHash> ObjectStoreMediaTracker;
    ObjectStoreMediaTracker objStoreDiskMediaTracker;
    ObjectStoreMediaTracker objStoreFlashMediaTracker;

    void initObjectStoreMediaErrorHandlers();

    /// returns ERR_OK if Object Store is available for IO
    Error checkAvailability() const;

    // Track when the last capacity message was sent
    float_t lastCapacityMessageSentAt;

  public:
    ObjectStore(const std::string &modName,
                SmIoReqHandler *data_store,
                StorMgrVolumeTable* volTbl,
                StartResyncFnObj fnObj=StartResyncFnObj(),
                DiskChangeFnObj diskChFnObj=DiskChangeFnObj(),
                TokenOfflineFnObj tokFn=TokenOfflineFnObj());
    ~ObjectStore();
    typedef std::unique_ptr<ObjectStore> unique_ptr;
    typedef std::shared_ptr<ObjectStore> ptr;

    /**
     * Returns the highest percentage of used capacity among all disks in non-all-SSD config.
     * Does not consider tiering data in calculations, but does include SM metadata stored in SSDs.
     * Returns 0 and logs an error if either the used capacity or the total capacity is 0.
     */
    float_t getUsedCapacityAsPct();

    /**
     * Open store for a given set of SM tokens. One or more
     * SM tokens may be already opened, which is ok.
     * Called when SM starts migrating new tokens for which it gained
     * ownership, so the metadata and data stores must be opened
     * This method does not update disk map; disk map is updated on
     * DLT update.
     */
    Error openStore(const SmTokenSet& smTokens);

    /**
     * Notification about DLT change
     * @return ERR_OK on success;
     *         ERR_SM_NOERR_NEED_RESYNC if StorMgr should start full token resync
     *         ERR_PERSIST_STATE_MISMATCH SM is just starting up and its persistent
     *         state did not match given DLT
     *         ERR_INVALID_ARG if ObjectStore already handled DLT with the same version
     */
    Error handleNewDlt(const DLT* dlt);

    /**
     * Notification about DLT close
     * This method handles losing ownership of SM tokens
     */
    Error handleDltClose(const DLT* dlt);

    /**
     * Adds a new volume to the object store. Some physical
     * resources are allocated, but new volume creation is
     * thinly provisioned.
     */
    Error addVolume(const VolumeDesc& volDesc);

    /**
     * Removes storage for a volume. Removes cache and other
     * per-volume structures.
     */
    Error removeVolume(fds_volid_t volId);

    /**
     * Peristently stores an object for a volume.
     */
    Error putObject(fds_volid_t volId,
                    const ObjectID &objId,
                    boost::shared_ptr<const std::string> objData,
                    fds_bool_t forwardedIO);

    /**
     * Gets an specific object for a volume. The object's data
     * is filled into the objData shared pointer parameter.
     * @param[out] usedTier tier from which data was read, if data
     * was read from cache, returns flash tier (this is for streaming
     * metadata)
     */
    boost::shared_ptr<const std::string> getObject(fds_volid_t volId,
                                                   const ObjectID &objId,
                                                   diskio::DataTier& usedTier,
                                                   Error& err);
    boost::shared_ptr<const std::string> getObjectData(fds_volid_t volId,
                                                   const ObjectID &objId,
                                                   ObjMetaData::const_ptr objMetaData,
                                                   Error& err);

    /**
     * Deletes a specific object. The object is marked as deleted,
     * but the actual data is deleted later by the garbage collector.
     */
    Error deleteObject(fds_volid_t volId,
                       const ObjectID &objId,
                       fds_bool_t forwardedIO);

    /**
     * Relocate/write back object from tier 'fromTier' to tier 'toTier'
     * @param[in] relocateFlag true means object will be removed from
     * 'fromTier' tier; false means object will be written to tier 'toTier'
     * and object will also remain on tier 'fromTier'
     */
    Error moveObjectToTier(const ObjectID& objId,
                           diskio::DataTier fromTier,
                           diskio::DataTier toTier,
                           fds_bool_t relocateFlag);

    /**
     * @brief Updates the object location from flash to disk
     * NOTE: This implementation is valid for beta-2 time.  As tiering evolves this code
     * needs to be re-visited.
     */
    Error updateLocationFromFlashToDisk(const ObjectID& objId,
                                        ObjMetaData::const_ptr objMeta);
    /**
     * Copies associated volumes from source to destination volume
     */
    Error copyAssociation(fds_volid_t srcVolId,
                          fds_volid_t destVolId,
                          const ObjectID& objId);

    /**
     * If given object still valid (refcount > 0), copies the object
     * to new file from the file that is being garbage collected
     * @param objOwned false if object is not owned by this SM anymore
     * and can be garbage collected even if refcnt > 0
     */
    Error copyObjectToNewLocation(const ObjectID& objId,
                                  diskio::DataTier tier,
                                  fds_bool_t verifyData,
                                  fds_bool_t objOwned);


    /**
     * Apply Object metadata/data from source SM
     * Object metadata and data may already exist, so may only
     * need to apply newest metadata. The metadata is overwritten with
     * the fields in the msg; the assumption here is that there is no
     * active IO on this SM for this object while the SM applying data
     * and metadata. This assumption is verified by TokenMigrationMgr
     * and MigrationExecutor.
     * @param objId object ID for which to apply data and metadata
     * @param msg thrift message from source SM containing data and
     *        metadata to apply
     * @return success or error
     */
    Error applyObjectMetadataData(const ObjectID& objId,
                                  const fpi::CtrlObjectMetaDataPropagate& msg);

    /**
     * Read data from given object metadata.
     */

    /**
     * Make a snapshot of metadata of given SM token and
     * calls notifFn method
     */
    void snapshotMetadata(fds_token_id smTokId,
                          SmIoSnapshotObjectDB::CbType notifFn,
                          SmIoSnapshotObjectDB* snapReq);

    /**
     * Make a persistent snapshot of metadata of given SM token and
     * calls notifFn method
     */
    void snapshotMetadata(fds_token_id smTokId,
                          SmIoSnapshotObjectDB::CbTypePersist notifFn,
                          SmIoSnapshotObjectDB* snapReq);


    /**
     * Returns number of disks on this SM
     */
    fds_uint32_t getDiskCount() const;

    /**
     * Handle disk change.
     */
    void handleDiskChanges(const DiskId& dId,
                           const diskio::DataTier& diskType,
                           const TokenDiskIdPairSet& tokenDiskPairs);

    /**
     * Handle detection of online disk failure.
     */
    Error handleOnlineDiskFailures(DiskId& diskId, const diskio::DataTier& tier);

    /**
     * Update the HDD and SSD disk trackers with any errors seen during
     * gets and puts.
     */
    void updateMediaTrackers(fds_token_id smTokId,
                             diskio::DataTier tier,
                             const Error& error);

    /**
     * Check if object store is ready to serve IO/become source for SM token
     * migration.
     */
    inline fds_bool_t isReady() const {
        return (currentState.load() == OBJECT_STORE_READY);
    }
    /**
     * Returns false if object store initialization failed and it is unavailable
     */
    inline fds_bool_t isUnavailable() const {
        return (currentState.load() == OBJECT_STORE_UNAVAILABLE);
    }

    // control methods
    Error scavengerControlCmd(SmScavengerCmd* scavCmd);
    Error tieringControlCmd(SmTieringCmd* tierCmd);

    Error SmCheckControlCmd(SmCheckCmd *checkCmd);
    void SmCheckUpdateDLT(const DLT *latestDLT);

    /**
     * Sets this ObjectStore to the UNAVAILABLE state
     */
    void setUnavailable();

    // FDS module control functions
    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTSTORE_H_
