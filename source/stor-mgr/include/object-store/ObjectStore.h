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
#include <object-store/ObjectDataStore.h>
#include <object-store/ObjectMetadataStore.h>
#include <utility>

namespace fds {

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

    /// Volume table for accessing vol descriptors
    // Does not own, passed from SM processing layer
    // TODO(xxx) we should be able to get this from platform
    StorMgrVolumeTable *volumeTbl;

    /// config params
    fds_bool_t conf_verify_data;
    fds_uint32_t numBitsPerToken;

    /// Task synchronizer
    std::unique_ptr<HashedLocks<ObjectID, ObjectHash>> taskSynchronizer;
    /// Size of the synchronizer (controls false positives)
    fds_uint32_t taskSyncSize;
    typedef ScopedHashedLock<ObjectID,
                             HashedLocks<ObjectID, ObjectHash>> ScopedSynchronizer;

  public:
    ObjectStore(const std::string &modName,
                SmIoReqHandler *data_store,
                StorMgrVolumeTable* volTbl);
    ~ObjectStore();
    typedef std::unique_ptr<ObjectStore> unique_ptr;
    typedef std::shared_ptr<ObjectStore> ptr;

    /**
     * Notification about DLT change
     */
    void handleNewDlt(const DLT* dlt);

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
                    boost::shared_ptr<const std::string> objData);

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
                       const ObjectID &objId);

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
     * Returns number of disks on this SM
     */
    fds_uint32_t getDiskCount() const;

    // control methods
    Error scavengerControlCmd(SmScavengerCmd* scavCmd);
    Error tieringControlCmd(SmTieringCmd* tierCmd);

    // FDS module control functions
    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTSTORE_H_
