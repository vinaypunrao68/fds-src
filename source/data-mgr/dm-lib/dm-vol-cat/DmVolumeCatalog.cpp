/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <list>
#include <set>
#include <string>
#include <dm-vol-cat/DmVolumeCatalog.h>

namespace fds {

DmVolumeCatalog::DmVolumeCatalog(char const *const name)
        : Module(name)
{
}

DmVolumeCatalog::~DmVolumeCatalog()
{
}

int DmVolumeCatalog::mod_init(SysParams const *const param)
{
    Module::mod_init(param);
    return 0;
}

void DmVolumeCatalog::mod_startup()
{
}

void DmVolumeCatalog::mod_shutdown()
{
}

//
// Allocates volume's specific datastrucs. Does not create
// persistent volume catalog file, because it could be potentially synced
// from other DM. After this call, DM Volume Catalog does not accept
// any get/put/delete requests for the volume yet.
//
Error DmVolumeCatalog::addCatalog(const VolumeDesc& voldesc)
{
    Error err(ERR_OK);
    LOGTRACE << "Will add Catalog for volume " << voldesc.name
             << ":" << std::hex << voldesc.volUUID << std::dec;
    return err;
}

//
// Prepares Volume Catalog (including cache and persistent layer) to
// accept get/put/delete requests for the given volume
//
Error DmVolumeCatalog::activateCatalog(fds_volid_t volume_id)
{
    Error err(ERR_OK);
    LOGTRACE << "Will activate catalog for volume " << std::hex
             << volume_id << std::dec;
    return err;
}

//
// Deletes each blob in the volume 'volume_id' and marks volume as deleted
//
Error DmVolumeCatalog::removeVolumeMeta(fds_volid_t volume_id)
{
    Error err(ERR_OK);
    return err;
}

//
// Returns true if the volume does not contain any valid blobs.
// A valid blob is a non-deleted blob version.
//
fds_bool_t DmVolumeCatalog::isVolumeEmpty(fds_volid_t volume_id) const
{
    return false;
}

//
// Updates committed blob in the Volume Catalog.
// Updates blob exluding list of offset to object id mappings.
//
Error
DmVolumeCatalog::putBlobMeta(const BlobMetaDesc::const_ptr& blob_meta)
{
    Error err(ERR_OK);
    LOGTRACE << "Will commit meta for " << *blob_meta;
    return err;
}

//
// Updates committed blob in the Volume Catalog.
// Updates blob metadata and a given list of offset to object id mappings
// (not necessarily the whole blob).
//
Error
DmVolumeCatalog::putBlob(const BlobMetaDesc::const_ptr& blob_meta,
                         const BlobObjList::const_ptr& blob_obj_list)
{
    Error err(ERR_OK);
    LOGTRACE << "Will commit blob " << *blob_meta << ";" << *blob_obj_list;
    return err;
}

//
// Flush blob to persistent storage. Will flush all blob extents.
//
Error
DmVolumeCatalog::flushBlob(fds_volid_t volume_id,
                           const std::string& blob_name)
{
    Error err(ERR_OK);
    return err;
}

//
// Returns blob metadata descriptor for the given blob 'blob_name'
// and volume 'volume_id'
//
BlobMetaDesc::const_ptr
DmVolumeCatalog::getBlobMeta(fds_volid_t volume_id,
                             const std::string& blob_name,
                             Error& result)
{
    LOGTRACE << "Will retrieve blob meta for blob " << blob_name
             << " volid " << std::hex << volume_id << std::dec;
    return NULL;
}

//
// Returns list of offset to object id mapping for the given
// blob 'blob_name' in the given volume and for the given list of offsets
//
BlobObjList::const_ptr
DmVolumeCatalog::getBlobObjects(fds_volid_t volume_id,
                                const std::string& blob_name,
                                const std::set<fds_uint64_t>& offset_list,
                                Error& result)
{
    LOGTRACE << "Will retrieve blob objects for blob " << blob_name
             << " volid " << std::hex << volume_id << std::dec;
    return NULL;
}

//
// Returns list of blobs in the volume 'volume_id'
//
Error DmVolumeCatalog::listBlobs(fds_volid_t volume_id,
                                 fds_uint32_t max_ret_blobs,
                                 fds_uint64_t* iterator_cookie,
                                 fpi::BlobInfoListType* bmeta_list)
{
    Error err(ERR_OK);
    return err;
}

Error DmVolumeCatalog::getBlobMeta(fds_volid_t volume_id,
                                   const std::string& blob_name,
                                   blob_version_t* blob_version,
                                   fds_uint64_t* blob_size,
                                   fpi::FDSP_MetaDataList* meta_list)
{
    Error err(ERR_OK);
    return err;
}

Error DmVolumeCatalog::getAllBlobObjects(fds_volid_t volume_id,
                                         const std::string& blob_name,
                                         blob_version_t* blob_version,
                                         fpi::FDSP_BlobObjectList* obj_list)
{
    Error err(ERR_OK);
    return err;
}

//
// Deletes the blob 'blob_name' verison 'blob_version'.
// If the blob version is invalid, deletes the most recent blob version.
//
Error DmVolumeCatalog::deleteBlob(fds_volid_t volume_id,
                                  const std::string& blob_name,
                                  blob_version_t blob_version)
{
    Error err(ERR_OK);
    return err;
}

DmVolumeCatalog gl_DmVolCatMod("Global DM Volume Catalog");

}  // namespace fds
