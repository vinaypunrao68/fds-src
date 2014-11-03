/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMVOLUMECATALOG_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMVOLUMECATALOG_H_

#include <string>
#include <list>
#include <set>
#include <vector>
#include <unordered_map>
#include <atomic>

#include <util/Log.h>
#include <fds_error.h>
#include <fds_module.h>
#include <DmBlobTypes.h>
#include <blob/BlobTypes.h>
#include <VcQueryIface.h>
#include <dm-vol-cat/DmExtentTypes.h>
#include <concurrency/RwLock.h>

namespace fds {

// forward declarations
class DmPersistVolCatalog;
class DmCacheVolCatalog;

struct DmVolumeDetails {
    typedef boost::shared_ptr<DmVolumeDetails> ptr;
    typedef boost::shared_ptr<const DmVolumeDetails> const_ptr;

    fds_volid_t volId;
    std::atomic<fds_uint64_t> size;
    std::atomic<fds_uint64_t> blobCount;
    std::atomic<fds_uint64_t> objectCount;

    DmVolumeDetails(fds_volid_t volId_, fds_uint64_t size_, fds_uint64_t blobCount_,
            fds_uint64_t objectCount_) : volId(volId_), size(size_), blobCount(blobCount_),
            objectCount(objectCount_) {}
};

typedef std::unordered_map<fds_volid_t, DmVolumeDetails::ptr> DmVolumeDetailsMap_t;

/**
 * This is the module that manages committed blob metadata
 * for all volumes managed by this DM. This module internally
 * manages two sub-modules: persistent layer of Volume catalog
 * that keeps per-volume catalogs in persistent storage and volume
 * catalog cache that sits on top of persistent layer.
 */
class DmVolumeCatalog : public Module,
                        public HasLogger,
                        public VolumeCatalogQueryIface {
  public:
    explicit DmVolumeCatalog(char const *const name);
    ~DmVolumeCatalog();
    typedef boost::shared_ptr<DmVolumeCatalog> ptr;
    typedef boost::shared_ptr<const DmVolumeCatalog> const_ptr;

    /**
     * Module methods
     */
    virtual int mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

    /**
     * Add catalog for a new volume described in 'voldesc'
     * When the function returns, the volume catalog for this volume
     * is inactive until activateCatalog() is called. For just added
     * volumes, activateCatalog() will be called right away. For already
     * existing volumes for which this DM takes a new responsibility,
     * the volume catalog remains inactive for some time while volume meta
     * is being synced from other DMs.
     * @param voldesc is a volume descriptor, which contains volume id, name
     * and other information about the volume
     */
    Error addCatalog(const VolumeDesc& voldesc);

    /**
     * Create copy of a volume
     */
    Error copyVolume(VolumeDesc & voldesc);

    /**
     * Activate catalog for the given volume 'volume_id'. After this call
     * Volume Catalog will accept put/get/delete requests for this volume
     */
    Error activateCatalog(fds_volid_t volume_id);

    /**
     * VolumeCatalogQueryIface methods. This interface is used by DM
     * processing payer to query for committed blob metadata.
     */
    /**
     * Callback to expunge a list of objects from a volume
     */
    void registerExpungeObjectsCb(expunge_objs_cb_t cb);

    /**
     * Marks volume as deleted if the volume does not contain any valid blobs.
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
     * Returns size of volume and number of blob in the volume 'volume_id'
     * @param[out] size size of volume in bytes
     * @param[in] blob_count number of blobs in the volume
     * @param[out] object_count object count per volume
     * @return ERR_OK on success, ERR_VOL_NOT_FOUND if volume is not known
     * to volume catalog
     */
    Error getVolumeMeta(fds_volid_t volume_id,
                        fds_uint64_t* size,
                        fds_uint64_t* blob_count,
                        fds_uint64_t* object_count);

    /**
     * Get all objects for the volume
     *
     * @param[in] volId volume identifier
     * @param[in,out] objIds set of all object ids for this volume
     *
     * @return ERR_OK on success
     */
    Error getVolumeObjects(fds_volid_t volId, std::set<ObjectID> & objIds);

    /**
     * Retrieves blob meta for the given blob_name and volume 'volume_id'
     * @param[in] volume_id volume uuid
     * @param[in] blob_name name of the blob
     * @param[in,out] blob_version version of the blob to retrieve, if not
     * set, the most recent version is retrieved. When the method returns,
     * blob_version is set to actual version that is retrieved
     * @param[out] blob_size size of blob in bytes
     * @param[out] meta_list list of metadata key-value pairs
     * @return ERR_OK on success, ERR_VOL_NOT_FOUND if volume is not known
     * to volume catalog
     */
    Error getBlobMeta(fds_volid_t volume_id,
                      const std::string& blob_name,
                      blob_version_t* blob_version,
                      fds_uint64_t* blob_size,
                      fpi::FDSP_MetaDataList* meta_list);

    /**
     * Retrieves all info about the blob with given blob_name and volume 'volume_id'
     * @param[in] volume_id volume uuid
     * @param[in] blob_name name of the blob
     * @param[in] start_offset starting blob offset being queried
     * @param[in] end_offset end blob offset being queried
     * @param[in,out] blob_version version of the blob to retrieve, if not
     * set, the most recent version is retrieved. When the method returns,
     * blob_version is set to actual version that is retrieved
     * @param[out] meta_list list of metadata key-value pairs
     * @param[out] obj_list list of offset to object id mappings
     * @return ERR_OK on success, ERR_VOL_NOT_FOUND if volume is not known
     * to volume catalog
     */
    Error getBlob(fds_volid_t volume_id,
                  const std::string& blob_name,
                  fds_uint64_t start_offset,
                  fds_int64_t end_offset,
                  blob_version_t* blob_version,
                  fpi::FDSP_MetaDataList* meta_list,
                  fpi::FDSP_BlobObjectList* obj_list);

    /**
     * Returns the list of blobs in the volume with basic blob info
     * @param[out] binfo_list list of blobs
     * @return ERR_OK on success, ERR_VOL_NOT_FOUND if volume is not known
     * to volume catalog
     */
    Error listBlobs(fds_volid_t volume_id,
                    fpi::BlobInfoListType* binfo_list);


    /**
     * Remaining methods are used by TVC only
     */

    /**
     * Updates committed blob in the Volume Catalog.
     * The method does not guarantee that on method return, the blob meta is
     * persisted. When the update is persisted, Volume Catalog will call
     * the callback to notify TVC that commit is persisted.
     * Each method contains meta list and (optionally) object list that
     * needs to be updated (not necesserily all the metadata and objects of
     * the blob).
     * @param[in] tx_id transaction id to be returned with callback when
     * Volume Catalog flushes the blob.
     */
    Error putBlobMeta(fds_volid_t volume_id,
                      const std::string& blob_name,
                      const MetaDataList::const_ptr& meta_list,
                      const BlobTxId::const_ptr& tx_id);
    Error putBlob(fds_volid_t volume_id,
                  const std::string& blob_name,
                  const MetaDataList::const_ptr& meta_list,
                  const BlobObjList::const_ptr& blob_obj_list,
                  const BlobTxId::const_ptr& tx_id);

    /**
     * Flushes given blob to the persistent storage. Blocking method, will
     * return when whole blob is flushed to persistent storage.
     */
    Error flushBlob(fds_volid_t volume_id,
                    const std::string& blob_name);

    /**
     * Deletes each blob in the volume and marks volume as deleted.
     */
    Error removeVolumeMeta(fds_volid_t volume_id);

    /**
     * Deletes the blob 'blob_name' verison 'blob_version'.
     * If the blob version is invalid, deletes the most recent blob version.
     */
    Error deleteBlob(fds_volid_t volume_id,
                     const std::string& blob_name,
                     blob_version_t blob_version);

    /**
     * Sync volume catalog to DM 'dm_uuid'
     */
    Error syncCatalog(fds_volid_t volume_id,
                      const NodeUuid& dm_uuid);
    /**
     * Get total matadata size for a volume
     */
     fds_uint64_t getTotalMetadataSize(fds_volid_t volume_id);

  private:
    /**
     * Internal function to get a blob's metadata extent, whether
     * it is in the cache or not.
     */
    BlobExtent0::ptr getMetaExtent(fds_volid_t volume_id,
                                   const std::string& blob_name,
                                   Error& error);

    /**
     * Internal function to update or add a new metadata extent
     * for a given volume and blob.
     */
    Error putMetaExtent(fds_volid_t volume_id,
                        const std::string& blob_name,
                        const BlobExtent0::const_ptr& meta_extent);

    /**
     * Internal function to get an extent for a blob, whether
     * it is in the cache or not.
     */
    BlobExtent::ptr getExtent(fds_volid_t volume_id,
                              const std::string& blob_name,
                              fds_extent_id extent_id,
                              Error& error);

    /**
     * Internal function to update extent 0 and a set of
     * non-0 extents in the volume catalog cache for given
     * volume id, blob name.
     */
    Error putExtents(fds_volid_t volume_id,
                     const std::string& blob_name,
                     const BlobExtent0::const_ptr& meta_extent,
                     const std::vector<BlobExtent::const_ptr>& extents);

    /**
     * Internal function to get blob count and size of the volume
     */
    Error getVolumeMetaInternal(fds_volid_t volume_id,
                        fds_uint64_t* size,
                        fds_uint64_t* blob_count,
                        fds_uint64_t* object_count);
    /**
     * Volume catalog cache layer module
     */
    std::unique_ptr<DmCacheVolCatalog> cacheCat;

    /**
     * Volume Catalog Persistent Layer module
     */
    DmPersistVolCatalog* persistCat;

    /**
     * Callback to expunge a list of objects when they
     * are dereferenced by a blob
     */
    expunge_objs_cb_t expunge_cb;

    DmVolumeDetailsMap_t volDetailsMap_;
    fds_rwlock lockVolDetailsMap_;
};
}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMVOLUMECATALOG_H_

