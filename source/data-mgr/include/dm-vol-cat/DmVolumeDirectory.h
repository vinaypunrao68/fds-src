/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMVOLUMEDIRECTORY_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMVOLUMEDIRECTORY_H_

#include <string>
#include <set>
#include <vector>

#include <fds_error.h>
#include <fds_types.h>
#include <fds_volume.h>
#include <util/Log.h>
#include <concurrency/RwLock.h>

#include <VcQueryIface.h>
#include <blob/BlobTypes.h>
#include <DmBlobTypes.h>
#include <dm-vol-cat/DmPersistVolDir.h>

namespace fds {

struct DmVolumeSummary {
    typedef boost::shared_ptr<DmVolumeSummary> ptr;
    typedef boost::shared_ptr<const DmVolumeSummary> const_ptr;

    fds_volid_t volId;
    std::atomic<fds_uint64_t> size;
    std::atomic<fds_uint64_t> blobCount;
    std::atomic<fds_uint64_t> objectCount;

    DmVolumeSummary(fds_volid_t volId_, fds_uint64_t size_, fds_uint64_t blobCount_,
            fds_uint64_t objectCount_) : volId(volId_), size(size_), blobCount(blobCount_),
            objectCount(objectCount_) {}
};

typedef std::unordered_map<fds_volid_t, DmVolumeSummary::ptr> DmVolumeSummaryMap_t;

/**
 * This modules stores all blob metadata and object ids for
 * all volumes managed by this DM. This module internally
 * uses two sub-modules: persistent layer and a cache that
 * sits on top of persistent layer
 */
class DmVolumeDirectory : public Module, public HasLogger,
        public VolumeCatalogQueryIface {
  public:
    typedef boost::shared_ptr<DmVolumeDirectory> ptr;
    typedef boost::shared_ptr<const DmVolumeDirectory> const_ptr;

    // static methods
    inline static fds_uint32_t getLastObjSize(fds_uint64_t blobSize, fds_uint32_t objSize) {
        fds_uint32_t rc = blobSize ? (blobSize % objSize) : 0;
        return (rc ? rc : objSize);
    }

    inline static fds_uint64_t getLastOffset(fds_uint64_t blobSize, fds_uint32_t objSize) {
        return (blobSize - getLastObjSize(blobSize, objSize));
    }

    // ctor and dtor
    explicit DmVolumeDirectory(char const * const name);
    ~DmVolumeDirectory();

    // Methods
    virtual void mod_startup() {}
    virtual void mod_shutdown() {}

    /**
     * Add catalog for a new volume described in 'voldesc'
     * When the function returns, the volume catalog for this volume
     * is inactive until activateCatalog() is called. For just added
     * volumes, activateCatalog() will be called right away. For existing
     * volumes for which this DM takes a new responsibility, the volume
     * catalog remains inactive for some time while volume meta is
     * being synced from other DMs.
     *
     * @param voldesc is a volume descriptor, which contains volume id, name
     *        and other information about the volume
     */
    Error addCatalog(const VolumeDesc & voldesc);

    /**
     * Create a copy of a volume
     */
    Error copyVolume(const VolumeDesc & voldesc);

    /**
     * Activate catalog for the given volume 'volId'. After this call
     * Volume Catalog will accept put/get/delete requests for this volume
     */
    Error activateCatalog(fds_volid_t volId);

    /**
     * VolumeCatalogQueryIface methods. This interface is used by DM
     * processing layer to query for committed blob metadata.
     */
    /**
     * Callback to expunge a list of objects from a volume
     */
    void registerExpungeObjectsCb(expunge_objs_cb_t cb);

    /**
     * Marks volume as deleted
     * @return ERR_OK
     */
    Error markVolumeDeleted(fds_volid_t volId);

    /**
     * Deletes catalog
     * @return ERR_OK if catalog was deleted; ERR_NOT_READY if volume is not marked
     * as deleted.
     */
    Error deleteEmptyCatalog(fds_volid_t volId);

    /**
     * Returns size of volume and number of blob in the volume 'volume_id'
     * @param[out] size size of volume in bytes
     * @param[in] blob_count number of blobs in the volume
     * @param[out] object_count object count per volume
     * @return ERR_OK on success, ERR_VOL_NOT_FOUND if volume is not known
     * to volume catalog
     */
    Error getVolumeMeta(fds_volid_t volId, fds_uint64_t* volSize, fds_uint64_t* blobCount,
            fds_uint64_t* objCount);

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
     * Retrieves blob meta for the given blobName and volume 'volId'
     * @param[in] volId volume identifier
     * @param[in] blobName name of the blob
     * @param[in] blobVersion version of the blob to retrieve, if not set,
     *            the most recent version is retrieved. When the method returns,
     *            blobVersion is set to actual version that is retrieved
     * @param[out] blobSize size of the blob
     * @param[out] metaList list of metadata key-value pairs
     * @return ERR_OK on success, ERR_VOL_NOT_FOUND if volume is not known
     */
    Error getBlobMeta(fds_volid_t volId, const std::string & blobName,
            blob_version_t * blobVersion, fds_uint64_t * blobSize,
            fpi::FDSP_MetaDataList * metaList);

    /**
     * Retrieves all info about the blob with given blobName and volume 'volId'
     * @param[in] volId volume uuid
     * @param[in] blobName name of the blob
     * @param[in] startOffset starting offset being queried
     * @param[in] endOffset end offset being queried
     * @param[in,out] blobVersion version of the blob to retrieve, if not set,
     *                the most recent version is retrieved. When the method returns,
     *                blobVersion is set to actual version that is retrieved
     * @param[out] metaList list of metadata key-value pairs
     * @param[out] objList list of offset to object id mappings
     * @return ERR_OK on success, ERR_VOL_NOT_FOUND if volume is not known
     */
    Error getBlob(fds_volid_t volId, const std::string& blobName, fds_uint64_t startOffset,
            fds_int64_t endOffset, blob_version_t* blobVersion, fpi::FDSP_MetaDataList* metaList,
            fpi::FDSP_BlobObjectList* objList);

    /**
     * Returns the list of blobs in the volume with basic blob info
     * @param[out] binfoList list of blobs
     * @return ERR_OK on success, ERR_VOL_NOT_FOUND if volume is not known
     */
    Error listBlobs(fds_volid_t volId, fpi::BlobInfoListType* binfoList);

    /**
     * Updates committed blob in the Volume Catalog.
     */
    Error putBlobMeta(fds_volid_t volId, const std::string& blobName,
            const MetaDataList::const_ptr& metaList, const BlobTxId::const_ptr& txId);
    Error putBlob(fds_volid_t volId, const std::string& blobName,
            const MetaDataList::const_ptr& metaList,
            const BlobObjList::const_ptr& blobObjList, const BlobTxId::const_ptr& txId);
    Error putBlob(fds_volid_t volId, const std::string& blobName, fds_uint64_t blobSize,
            const MetaDataList::const_ptr& metaList,
            CatWriteBatch & wb, bool truncate = true);

    /**
     * Flushes given blob to the persistent storage. Blocking method, will
     * return when whole blob is flushed to persistent storage.
     */
    Error flushBlob(fds_volid_t volId, const std::string& blobName);

    /**
     * Deletes each blob in the volume and marks volume as deleted.
     */
    Error removeVolumeMeta(fds_volid_t volId);

    /**
     * Deletes the blob 'blobName' verison 'blobVersion'.
     * If the blob version is invalid, deletes the most recent blob version.
     */
    Error deleteBlob(fds_volid_t volId, const std::string& blobName,
            blob_version_t blobVersion);

    /**
     * Sync volume catalog to DM 'dmUuid'
     */
    Error syncCatalog(fds_volid_t volId, const NodeUuid& dmUuid);

    /**
     * Get total matadata size for a volume
     */
     fds_uint64_t getTotalMetadataSize(fds_volid_t volId) {
         // TODO(umesh): implement this
         return 0;
     }

  private:
    // methods
    Error getBlobMetaDesc(fds_volid_t volId, const std::string & blobName,
            BlobMetaDesc & blobMeta);

    Error getVolumeMetaInternal(fds_volid_t volId, fds_uint64_t * volSize,
            fds_uint64_t * blobCount, fds_uint64_t * objCount);

    inline void mergeMetaList(MetaDataList & lhs, const MetaDataList & rhs) {
        for (auto & it : rhs) {
            lhs[it.first] = it.second;
        }
    }

    // vars
    std::unordered_map<fds_volid_t, DmPersistVolDir::ptr> volMap_;
    fds_rwlock volMapLock_;

    expunge_objs_cb_t expungeCb_;

    DmVolumeSummaryMap_t volSummaryMap_;
    fds_rwlock lockVolSummaryMap_;
};
}  // namespace fds
#endif  // SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMVOLUMEDIRECTORY_H_
