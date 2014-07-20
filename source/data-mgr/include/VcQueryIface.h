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
     * Retrieves all info about the blob with given blob_name and volume 'volume_id'
     * @param[in] volume_id volume uuid
     * @param[in] blob_name name of the blob
     * @param[in,out] blob_version version of the blob to retrieve, if not
     * set, the most recent version is retrieved. When the method returns,
     * blob_version is set to actual version that is retrieved
     * @param[out] blob_size size of blob in bytes
     * @param[out] meta_list list of metadata key-value pairs
     * @param[out] obj_list list of offset to object id mappings
     */
    virtual Error getBlob(fds_volid_t volume_id,
                          const std::string& blob_name,
                          blob_version_t* blob_version,
                          fds_uint64_t* blob_size,
                          fpi::FDSP_MetaDataList* meta_list,
                          fpi::FDSP_BlobObjectList* obj_list) = 0;

    /**
     * Returns the list of all blobs in the volume with basic blob info
     * @param[out] binfo_list list of blobs
     */
    virtual Error listBlobs(fds_volid_t volume_id,
                            fpi::BlobInfoListType* binfo_list) = 0;

    /**
     * Sync snapshot of volume catalog to dm 'dm_uuid'
     */
    virtual Error syncCatalog(fds_volid_t volume_id,
                              const NodeUuid& dm_uuid) = 0;
};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_VCQUERYIFACE_H_

