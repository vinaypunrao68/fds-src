/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_VCQUERYIFACE_H_
#define SOURCE_DATA_MGR_INCLUDE_VCQUERYIFACE_H_

#include <string>
#include <vector>
#include <fds_error.h>
#include "fdsp/dm_api_types.h"
#include <lib/Catalog.h>
#include <functional>
#include <DmBlobTypes.h>
namespace fds {

/**
 * Callback type to expunge a list of objects
 */
typedef std::function<Error (fds_volid_t volid,
                             const std::vector<ObjectID>& oids, bool)> expunge_objs_cb_t;

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
    virtual Error statVolume(fds_volid_t volume_id,
                             fds_uint64_t* size,
                             fds_uint64_t* blob_count,
                             fds_uint64_t* object_count) = 0;

    /**
     * Returns key-value metadata for the volume 'volume_id'
     * @param[in]  volume_id ID of the volume to get metadata from
     * @param[out] metadataList list of metadata for the volume
     * @return ERR_OK on success; ERR_VOL_NOT_FOUND is volume is not known
     * to volume catalog
     */
    virtual Error getVolumeMetadata(fds_volid_t volume_id,
                                    fpi::FDSP_MetaDataList &metadataList) = 0;

    /**
     * Closes catalog for a volume and reopens it
     * @param[in] voldesc volume descriptor
     * @return ERR_OK on success
     */
    virtual Error reloadCatalog(const VolumeDesc & voldesc) = 0;

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
     * @param[in] start_offset starting blob offset being queried
     * @param[in] end_offset ending blob offset being queried
     * @param[in,out] blob_version version of the blob to retrieve, if not
     * set, the most recent version is retrieved. When the method returns,
     * blob_version is set to actual version that is retrieved
     * @param[out] meta_list list of metadata key-value pairs
     * @param[out] obj_list list of offset to object id mappings
     * @param[out] blob_size size of blob in bytes
     * @return ERR_OK on success; ERR_VOL_NOT_FOUND is volume is not known
     * to volume catalog
     */
    virtual Error getBlob(fds_volid_t volume_id,
                          const std::string& blob_name,
                          fds_uint64_t start_offset,
                          fds_int64_t end_offset,
                          blob_version_t* blob_version,
                          fpi::FDSP_MetaDataList* meta_list,
                          fpi::FDSP_BlobObjectList* obj_list,
                          fds_uint64_t* blob_size) = 0;

    /**
     * Returns the list of all blobs in the volume with basic blob info
     * @param[out] binfo_list list of blobs
     * @return ERR_OK on success; ERR_VOL_NOT_FOUND is volume is not known
     * to volume catalog
     */
    virtual Error listBlobs(fds_volid_t volume_id,
                            fpi::BlobDescriptorListType* bdescr_list) = 0;

    virtual Error listBlobsWithPrefix (fds_volid_t volume_id,
                                       std::string const& prefix,
                                       std::string const& delimiter,
                                       fpi::BlobDescriptorListType& results) = 0;

    /**
     * Returns blob (descriptor + offset to object_id mappings) for a blob_id
     * intended to be used for logical replication
     * @param[in] volume_id volume uuid
     * @param[in] blob_id id of the blob
     * @param[out] meta the blob metadata descriptor
     * @param[out] obj_list list of offset to object id mappings
     * @param[in] m the snapshot to read from
     * @return ERR_OK on success; ERR_VOL_NOT_FOUND is volume is not known
     * to volume catalog
     */
    virtual Error getBlobAndMetaFromSnapshot(fds_volid_t volume_id,
                                             const std::string & blobName,
                                             BlobMetaDesc &meta,
                                             fpi::FDSP_BlobObjectList& obj_list,

                                             Catalog::MemSnap snap) = 0;
    /**
     * Sync snapshot of volume catalog to dm 'dm_uuid'
     */
    virtual Error syncCatalog(fds_volid_t volume_id,
                              const NodeUuid& dm_uuid) = 0;

    virtual Error deleteBlob(fds_volid_t volume_id,
                     const std::string& blob_name,
                     blob_version_t blob_version,
                     bool const expunge_data = true) = 0;
    /**
     * get total matadata size for a volume
     */
    virtual fds_uint64_t getTotalMetadataSize(fds_volid_t volume_id) = 0;

    virtual Error getVolumeSequenceId(fds_volid_t volId, sequence_id_t& seq_id) = 0;

    virtual Error activateCatalog(fds_volid_t volId) {
        return ERR_OK;
    };

    /**
     * Methods for DM Migration to take a volume's snapshot and do delta on it
     */
    virtual Error getVolumeSnapshot(fds_volid_t volId, Catalog::MemSnap &m) = 0;
    virtual Error freeVolumeSnapshot(fds_volid_t volId, Catalog::MemSnap &m) = 0;
    virtual Error getAllBlobsWithSequenceId(fds_volid_t volId,
                                            std::map<std::string, int64_t>& blobsSeqId,
                                            Catalog::MemSnap m = NULL) = 0;

    virtual Error forEachObject(fds_volid_t volId, std::function<void(const ObjectID&)>) = 0;

    /**
     * Put objects offset
     */
    virtual Error putObject(fds_volid_t volId,
                            const std::string & blobName,
                            const BlobObjList & objs) = 0;
};

}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_VCQUERYIFACE_H_
