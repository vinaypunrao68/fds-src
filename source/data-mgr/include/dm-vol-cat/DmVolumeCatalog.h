/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMVOLUMECATALOG_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMVOLUMECATALOG_H_

#include <string>
#include <list>
#include <util/Log.h>
#include <fds_error.h>
#include <fds_module.h>
#include <DmBlobTypes.h>

namespace fds {

/**
 * This is the module that manages committed blob metadata
 * for all volumes managed by this DM. This module internally
 * manages two sub-modules: persistent layer of Volume catalog
 * that keeps per-volume catalogs in persistent storage and volume
 * catalog cache that sits on top of persistent layer.
 */
class DmVolumeCatalog: public Module, public HasLogger {
  public:
        explicit DmVolumeCatalog(char const *const name);
        ~DmVolumeCatalog();

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
         * Deletes each blob in the volume and marks volume as deleted.
         */
        Error removeVolumeMeta(fds_volid_t volume_id);

        /**
         * Returns true if the volume does not contain any valid blobs.
         * A valid blob is a non-deleted blob version
         */
        fds_bool_t isVolumeEmpty() const;

        /**
         * Synchronous versions of updating committed blob in the Volume Catalog.
         * The methods return when blob update is written to persistent storage.
         * Each method contains meta list and (optionally) object list that
         * needs to be updated (not necesserily all the metadata and objects of
         * the blob).
         */
        Error putBlobMetaSync(const BlobMetaDesc::const_ptr& blob_meta);
        Error putBlobSync(const BlobMetaDesc::const_ptr& blob_meta,
                          const BlobObjList::const_ptr& blob_obj_list);

        /**
         * Retrieves blob meta descriptor for the given blob_name in volume
         * 'volume_id'
         */
        BlobMetaDesc::const_ptr getBlobMeta(fds_volid_t volume_id,
                                            const std::string& blob_name,
                                            Error& result);
        BlobObjList::const_ptr getAllBlobObjects(fds_volid_t volume_id,
                                                 const std::string& blob_name,
                                                 Error& result);
        // TODO(xxx) what data struct is used in tvc for list of offsets?
        BlobObjList::const_ptr getBlobObjects(fds_volid_t volume_id,
                                              const std::string& blob_name,
                                              Error& result);

        /**
         * Returns the list of blobs in the volume, where each entry in the list
         * is a blob metadata descriptor
         */
        Error listBlobs(fds_volid_t volume_id,
                        std::list<BlobMetaDesc>& bmeta_list);

        /**
         * Deletes the blob 'blob_name' verison 'blob_version'.
         * If the blob version is invalid, deletes the most recent blob version.
         */
        Error deleteBlob(fds_volid_t volume_id,
                         const std::string& blob_name,
                         blob_version_t blob_version);

  private:
};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMVOLUMECATALOG_H_

