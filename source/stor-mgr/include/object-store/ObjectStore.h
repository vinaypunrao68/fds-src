/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTSTORE_H_
#define SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTSTORE_H_

#include <string>
#include <fds_module.h>
#include <fds_volume.h>
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

  public:
    explicit ObjectStore(const std::string &modName);
    ~ObjectStore();
    typedef std::unique_ptr<ObjectStore> unique_ptr;

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
    Error getObject(fds_volid_t volId,
                    const ObjectID &objId,
                    boost::shared_ptr<std::string> objData);

    /**
     * Deletes a specific object. The object's data is filled into
     * the objData shared pointer parameter.
     */
    Error deleteObject(fds_volid_t volId,
                       const ObjectID &objId);

    // FDS module control functions
    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();
};

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_OBJECT_STORE_OBJECTSTORE_H_
