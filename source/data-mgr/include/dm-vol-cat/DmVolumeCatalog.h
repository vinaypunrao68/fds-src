/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMVOLUMECATALOG_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMVOLUMECATALOG_H_

#include <string>
#include <list>
#include <set>
#include <util/Log.h>
#include <fds_error.h>
#include <fds_module.h>
#include <DmBlobTypes.h>
#include <blob/BlobTypes.h>
#include <VcQueryIface.h>
#include <dm-vol-cat/DmExtentTypes.h>

namespace fds {

    // forward declarations
    class DmPersistVolCatalog;

    /**
     * This is the module that manages committed blob metadata
     * for all volumes managed by this DM. This module internally
     * manages two sub-modules: persistent layer of Volume catalog
     * that keeps per-volume catalogs in persistent storage and volume
     * catalog cache that sits on top of persistent layer.
     */
    class DmVolumeCatalog:
    public Module, public HasLogger, public VolumeCatalogQueryIface {
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
         * Returns true if the volume does not contain any valid blobs.
         * A valid blob is a non-deleted blob version
         */
        fds_bool_t isVolumeEmpty(fds_volid_t volume_id) const;

        /**
         * Retrieves blob meta for the given blob_name and volume 'volume_id'
         * @param[in] volume_id volume uuid
         * @param[in] blob_name name of the blob
         * @param[in,out] blob_version version of the blob to retrieve, if not
         * set, the most recent version is retrieved. When the method returns,
         * blob_version is set to actual version that is retrieved
         * @param[out] blob_size size of blob in bytes
         * @param[out] meta_list list of metadata key-value pairs
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
         * @param[in,out] blob_version version of the blob to retrieve, if not
         * set, the most recent version is retrieved. When the method returns,
         * blob_version is set to actual version that is retrieved
         * @param[out] meta_list list of metadata key-value pairs
         * @param[out] obj_list list of offset to object id mappings
         */
        Error getBlob(fds_volid_t volume_id,
                      const std::string& blob_name,
                      blob_version_t* blob_version,
                      fds_uint64_t* blob_size,
                      fpi::FDSP_MetaDataList* meta_list,
                      fpi::FDSP_BlobObjectList* obj_list);

        /**
         * Returns the list of blobs in the volume with basic blob info
         * @param[out] binfo_list list of blobs
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

  private:
        /**
         * Volume Catalog Persistent Layer module
         */
        DmPersistVolCatalog* persistCat;

        /**
         * Callback to expunge a list of objects when they
         * are dereferenced by a blob
         */
        expunge_objs_cb_t expunge_cb;
    };

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMVOLUMECATALOG_H_

