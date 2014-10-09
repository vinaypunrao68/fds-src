/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_VCQUERYIFACE_H_
#define SOURCE_DATA_MGR_INCLUDE_VCQUERYIFACE_H_

#include <string>
#include <vector>
#include <fds_error.h>

namespace fds {

/**
 * Callback type to expunge a list of objects
 */
typedef std::function<Error (fds_volid_t volid,
                             const std::vector<ObjectID>& oids)> expunge_objs_cb_t;

/**
 * Interface to Volume Catalog for querying commited versions of
 * blob metadata in volume catalogs.
 */
class VolumeCatalogQueryIface {
  public:
    typedef boost::shared_ptr<VolumeCatalogQueryIface> ptr;
    typedef boost::shared_ptr<const VolumeCatalogQueryIface> const_ptr;
    virtual ~VolumeCatalogQueryIface() {}

    /**
     * Callback to expunge a list of objects from a volume
     */
    virtual void registerExpungeObjectsCb(expunge_objs_cb_t cb) = 0;

    /**
     * Returns size of volume and number of blob in the volume 'volume_id'
     * @param[out] size size of volume in bytes
     * @param[out] blob_count number of blobs in the volume
     * @param[out] object_count number of objects in the volume
     * @return ERR_OK on success; ERR_VOL_NOT_FOUND is volume is not known
     * to volume catalog
     */
    virtual Error getVolumeMeta(fds_volid_t volume_id,
                                fds_uint64_t* size,
                                fds_uint64_t* blob_count,
                                fds_uint64_t* object_count) = 0;

    /**
     * Retrieves blob meta for the given blob_name and volume 'volume_id'
     * @param[in] volume_id volume uuid
     * @param[in] blob_name name of the blob
     * @param[in,out] blob_version version of the blob to retrieve, if not
     * set, the most recent version is retrieved. When the method returns,
     * blob_version is set to actual version that is retrieved
     * @param[out] blob_size size of blob in bytes
     * @param[out] meta_list list of metadata key-value pairs
     * @return ERR_OK on success; ERR_VOL_NOT_FOUND is volume is not known
     * to volume catalog
     */
    virtual Error getBlobMeta(fds_volid_t volume_id,
                              const std::string& blob_name,
                              blob_version_t* blob_version,
                              fds_uint64_t* blob_size,
                              fpi::FDSP_MetaDataList* meta_list) = 0;

    /**
     * Retrieves all info about the blob with given blob_name and volume 'volume_id'
     * @param[in] volume_id volume uuid
     * @param[in] blob_name name of the blob
     * @param[in] blob_offset blob offset being queried
     * @param[in,out] blob_version version of the blob to retrieve, if not
     * set, the most recent version is retrieved. When the method returns,
     * blob_version is set to actual version that is retrieved
     * @param[out] meta_list list of metadata key-value pairs
     * @param[out] obj_list list of offset to object id mappings
     * @return ERR_OK on success; ERR_VOL_NOT_FOUND is volume is not known
     * to volume catalog
     */
    virtual Error getBlob(fds_volid_t volume_id,
                          const std::string& blob_name,
                          fds_uint64_t blob_offset,
                          blob_version_t* blob_version,
                          fpi::FDSP_MetaDataList* meta_list,
                          fpi::FDSP_BlobObjectList* obj_list) = 0;

    /**
     * Returns the list of all blobs in the volume with basic blob info
     * @param[out] binfo_list list of blobs
     * @return ERR_OK on success; ERR_VOL_NOT_FOUND is volume is not known
     * to volume catalog
     */
    virtual Error listBlobs(fds_volid_t volume_id,
                            fpi::BlobInfoListType* binfo_list) = 0;

    /**
     * Sync snapshot of volume catalog to dm 'dm_uuid'
     */
    virtual Error syncCatalog(fds_volid_t volume_id,
                              const NodeUuid& dm_uuid) = 0;

    virtual Error deleteBlob(fds_volid_t volume_id,
                     const std::string& blob_name,
                     blob_version_t blob_version) = 0;
    /**
     * get  vvc mata size
     */
     virtual fds_uint32_t  getVvcMetaSize(fds_volid_t volume_id) = 0;
};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_VCQUERYIFACE_H_

