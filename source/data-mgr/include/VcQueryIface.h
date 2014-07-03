/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_VCQUERYIFACE_H_
#define SOURCE_DATA_MGR_INCLUDE_VCQUERYIFACE_H_

#include <string>
#include <fds_error.h>

namespace fds {

/**
 * Interface to Volume Catalog for querying commited versions of
 * blob metadata in volume catalogs. Includes methods for creating
 * volume catalogs 
 */
    class VolumeCatalogQueryIface {
  public:
        virtual ~VolumeCatalogQueryIface() {}

        /**
         * Add catalog for a new volume described in 'voldesc'
         * When the function returns, the volume catalog for this volume
         * is inactive, i.e. queries to volume catalog wwill fail.
         * DM processing layer will need to call activateCatalog() to actually
         * activate catalog for new volumes. For already existing volumes, for
         * which DM takes a new responsibility, the volume catalog remains inactive
         * while volume catalog sync is in progress; in that case, migration
         * module will activate volume catalog when volume catalog is ready for
         * access.
         * @param voldesc is a volume descriptor, which contains volume id, name
         * and other information about the volume
         */
        virtual Error addCatalog(const VolumeDesc& voldesc) = 0;

        /**
         * Activate catalog for the given volume 'volume_id'. After this call
         * Volume Catalog will accept queries for this volume
         */
        virtual Error activateCatalog(fds_volid_t volume_id) = 0;

        /**
         * Returns true if the volume does not contain any valid blobs.
         * A valid blob is a non-deleted blob version
         */
        virtual fds_bool_t isVolumeEmpty(fds_volid_t volume_id) const = 0;

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
        virtual Error getBlobMeta(fds_volid_t volume_id,
                                  const std::string& blob_name,
                                  blob_version_t* blob_version,
                                  fds_uint64_t* blob_size,
                                  fpi::FDSP_MetaDataList* meta_list) = 0;

        /**
         * Retrieves blob meta for the given blob_name and volume 'volume_id'
         * @param[in] volume_id volume uuid
         * @param[in] blob_name name of the blob
         * @param[in,out] blob_version version of the blob to retrieve, if not
         * set, the most recent version is retrieved. When the method returns,
         * blob_version is set to actual version that is retrieved
         * @param[out] obj_list list of offset to object id mappings
         */
        virtual Error getAllBlobObjects(fds_volid_t volume_id,
                                        const std::string& blob_name,
                                        blob_version_t* blob_version,
                                        fpi::FDSP_BlobObjectList* obj_list) = 0;

        /**
         * Returns the list of blobs in the volume with basic blob info
         * @param[in] max_ret_blobs maximum number of blobs to return; if
         * total number of blobs in the volume exceeds max_ret_blobs, returns
         * max_ret_blobs blobs
         * @param[in,out] iterator_cookie keeps the blob number to start with,
         * when method returns iterator_cookie is updated with cookie to use
         * in the next function call to continue with next set of blobs
         * @param[out] binfo_list list of blobs
         * @return ERR_END_OF_LIST if there are no more blobs to return
         */
        virtual Error listBlobs(fds_volid_t volume_id,
                                fds_uint32_t max_ret_blobs,
                                fds_uint64_t* iterator_cookie,
                                fpi::BlobInfoListType* binfo_list) = 0;
};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_VCQUERYIFACE_H_

