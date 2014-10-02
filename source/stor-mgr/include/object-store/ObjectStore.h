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
#include <object-store/ObjectDataStore.h>
#include <object-store/ObjectMetadataStore.h>

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
                StorMgrVolumeTable* volTbl);
    ~ObjectStore();
    typedef std::unique_ptr<ObjectStore> unique_ptr;

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
     */
    boost::shared_ptr<const std::string> getObject(fds_volid_t volId,
                                                   const ObjectID &objId,
                                                   Error& err);

    /**
     * Deletes a specific object. The object is marked as deleted,
     * but the actual data is deleted later by the garbage collector.
     */
    Error deleteObject(fds_volid_t volId,
                       const ObjectID &objId);

    /**
     * Copies associated volumes from source to destination volume
     */
    Error copyAssociation(fds_volid_t srcVolId,
                          fds_volid_t destVolId,
                          const ObjectID& objId);

    /**
     * If given object still valid (refcount > 0), copies the object
     * to new file from the file that is being garbage collected
     */
    Error copyObjectToNewLocation(const ObjectID& objId,
                                  diskio::DataTier tier);

    // FDS module control functions
    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTSTORE_H_
