/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMPERSISTVC_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMPERSISTVC_H_

#include <string>
#include <vector>
#include <util/Log.h>
#include <fds_error.h>
#include <fds_module.h>
#include <concurrency/RwLock.h>
#include <dm-vol-cat/DmExtentTypes.h>

#define  MAX_EXTENT0_OBJS 1024
#define  MAX_EXTENT_OBJS  2048
namespace fds {

class PersistVolumeMeta;
typedef std::shared_ptr<PersistVolumeMeta> PersistVolumeMetaPtr;

/**
 * Volume Catalog persistent layer
 * Provides access to blob metadata in terms of extents. Each
 * blob is devided into one or more extents, where each extent
 * contains configurable range of offsets (offset to object info).
 * This is a per-volume configuration in persistent layer.
 */
class DmPersistVolCatalog: public Module, public HasLogger {
  public:
    explicit DmPersistVolCatalog(char const *const name);
    ~DmPersistVolCatalog();

    /**
     * Module methods
     */
    virtual int mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

    /**
     * Creates volume catalog for volume described in
     * volume descriptor 'vol_desc'.
     */
    Error createCatalog(const VolumeDesc& vol_desc);

    /**
     * Create snapshot for volume specified by id
     */
    Error copyVolume(fds_volid_t volId, fds_volid_t snapshotId,
           fds_bool_t snapshot,  fds_bool_t readOnly);

    /**
     * Opens catalog for volume 'volume_id'. When this method
     * returns, persistent layer accepts operations on persistent
     * catalog such as put/get/delete
     */
    Error openCatalog(fds_volid_t volume_id);

    /**
     * Mark volume as deleted if the volume does not contain any valid blobs.
     * A valid blob is a non-deleted blob version
     * @return ERR_OK if volume is empty and marked as deleted;
     *  ERR_VOL_NOT_EMPTY if volume contains at least on valid blob
     */
    Error markVolumeDeleted(fds_volid_t volume_id);

    /**
     * Deletes catalog that has not valid blobs. A valid blob is a non-deleted blob
     * version
     * @return ERR_OK if catalog was deleted; ERR_NOT_READY if volume is not marked
     * as deleted.
     */
    Error deleteEmptyCatalog(fds_volid_t volume_id);

    /**
     * Maps offset in bytes to extent id for a given volume id.
     * Volume id is required because the mapping depends on volume
     * specific policy such as max object size, and also volume
     * specific configuration for persistent volume catalog, such as
     * maximum number of entries in an extent.
     * @param[in] volume_id is id of the volume
     * @param[in] offset_bytes is an offset in bytes
     * @return extent id that contains given offset in bytes
     */
    fds_extent_id getExtentId(fds_volid_t volume_id,
                              fds_uint64_t offset_bytes);

    /**
     * Update or add extent 0 to the persistent volume catalog
     * for given volume_id,blob_name
     */
    Error putMetaExtent(fds_volid_t volume_id,
                        const std::string& blob_name,
                        const BlobExtent0::const_ptr& meta_extent);

    /**
     * Updates extent 0 and a set of non-0 extents in the persistent
     * volume catalog for given volume id, blob name.
     * This update is atomic -- either all updates are applied or none.
     * If extent 0 or any other extens do not exist, they are added to the DB
     * If any non-zero extent contains no offset to object info mappings, this
     * extent is deleted from the DB.
     */
    Error putExtents(fds_volid_t volume_id,
                     const std::string& blob_name,
                     const BlobExtent0::const_ptr& meta_extent,
                     const std::vector<BlobExtent::const_ptr>& extents);


    /**
     * Retrieves extent 0 from the persistent layer for given
     * volume id, blob_name. If extent is not found, returns allocated
     * BlobExtent0 containing correct extent configuration such as first offset,
     * max offset entries, etc.
     * @param[out] error will contain ERR_OK on success; ERR_CAT_ENTRY_NOT_FOUND
     * if extent0 does not exist in the persistent layer; or other error.
     * @return BlobExtent0 containing extent from persistent layer; If
     * err is ERR_NOT_FOUND, BlobExtent0 is allocated but not filled in;
     * returns null pointer in other cases.
     */
    BlobExtent0::ptr getMetaExtent(fds_volid_t volume_id,
                                   const std::string& blob_name,
                                   Error& error);

    /**
     * Retrieves extent with id 'extent_id' from the persistent layer for given
     * volume id, blob_name. If extent is not found, returns allocated BlobExtent
     * containing corrent extent configuration such as first offset, max offset
     * entries, etc.
     * @param[out] error will contain ERR_OK on success; ERR_NOT_FOUND
     * if extent0 does not exist in the persistent layer; or other error.
     * @return BlobExtent containing extent from persistent layer; If
     * err is ERR_NOT_FOUND, BlobExtent0 is allocated but not filled in;
     * returns null pointer in other cases.
     */
    BlobExtent::ptr getExtent(fds_volid_t volume_id,
                              const std::string& blob_name,
                              fds_extent_id extent_id,
                              Error& error);

    /**
     * Retrieves list of basic metadata info for all the blobs in
     * volume 'volume_id'
     */
    Error getMetaDescList(fds_volid_t volume_id,
                          std::vector<BlobExtent0Desc>& desc_list);

    /**
     * Deletes extent 'extent id' of blob 'blob_name' in volume
     * catalog of volume 'volume_id'
     */
    Error deleteExtent(fds_volid_t volume_id,
                       const std::string& blob_name,
                       fds_extent_id extent_id);

    /**
     * Sync volume catalog to destination DM 'dm_uuid'
     */
    Error syncCatalog(fds_volid_t volume_id,
                      const NodeUuid& dm_uuid);

  private:  // methods
    /**
     * returns shared pointer to volume meta for 'volume_id'
     */
    PersistVolumeMetaPtr getVolumeMeta(fds_volid_t volume_id);

    /**
     * @param[out] serialized_key key to volume catalog db for
     * given blob_name and extent_id
     */
    Error getKeyString(const std::string &blob_name,
                       fds_extent_id extent_id,
                       std::string& serialized_key);

  private:
    std::unordered_map<fds_volid_t, PersistVolumeMetaPtr> vol_map;
    fds_rwlock vol_map_lock;
};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMPERSISTVC_H_
