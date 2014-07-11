/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <list>
#include <set>
#include <vector>
#include <string>
#include <dm-vol-cat/DmVolumeCatalog.h>
#include <dm-vol-cat/DmPersistVc.h>

namespace fds {

DmVolumeCatalog::DmVolumeCatalog(char const *const name)
        : Module(name)
{
    persistCat = new DmPersistVolCatalog("DM Vol Cat Persistent Layer");
    static Module* om_mods[] = {
        persistCat,
        NULL
    };
}

DmVolumeCatalog::~DmVolumeCatalog()
{
    delete persistCat;
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
    LOGDEBUG << "Will add Catalog for volume " << voldesc.name
             << ":" << std::hex << voldesc.volUUID << std::dec;

    // TODO(xxx) allocate caches space for volume, etc.

    // create volume catalog in VC persistent layer
    err = persistCat->createCatalog(voldesc);
    return err;
}

//
// Prepares Volume Catalog (including cache and persistent layer) to
// accept get/put/delete requests for the given volume
//
Error DmVolumeCatalog::activateCatalog(fds_volid_t volume_id)
{
    Error err(ERR_OK);
    LOGDEBUG << "Will activate catalog for volume " << std::hex
             << volume_id << std::dec;

    // TODO(xxx) activate cache?

    // initialized VC in persistent layer
    err = persistCat->openCatalog(volume_id);
    return err;
}

//
// Deletes each blob in the volume 'volume_id' and marks volume as deleted
//
Error DmVolumeCatalog::removeVolumeMeta(fds_volid_t volume_id)
{
    Error err(ERR_OK);
    LOGDEBUG << "Will remove volume meta for volune " << std::hex
             << volume_id << std::dec;

    // TODO(xxx) implement
    return err;
}

//
// Returns true if the volume does not contain any valid blobs.
// A valid blob is a non-deleted blob version.
//
fds_bool_t DmVolumeCatalog::isVolumeEmpty(fds_volid_t volume_id) const
{
    // TODO(xxx) implement
    return false;
}

//
// Updates committed blob in the Volume Catalog.
// Updates blob exluding list of offset to object id mappings.
//
Error
DmVolumeCatalog::putBlobMeta(const BlobMetaDesc::const_ptr& blob_meta,
                             const BlobTxId::const_ptr& tx_id)
{
    Error err(ERR_OK);
    LOGTRACE << "Will commit meta for " << *blob_meta;

    // TODO(xxx) when we incorporate cache, we should add private
    // methods to get extent which will read from cache first and
    // then from persistent layer if not found

    // blob meta is in extent 0, so retrieve extent 0
    BlobExtent0::ptr extent0 = persistCat->getMetaExtent(blob_meta->vol_id,
                                                         blob_meta->blob_name,
                                                         err);

    // even if not found, we get an allocated extent0 which we will
    // fill in
    if (err.ok() || (err == ERR_CAT_ENTRY_NOT_FOUND)) {
        LOGTRACE << "Retrieved extent 0 for volume " << std::hex
                 << blob_meta->vol_id << std::dec << " " << *extent0
                 << " error " << err;

        fds_verify((err == ERR_CAT_ENTRY_NOT_FOUND) ||
                   (extent0->blobVersion() != blob_version_invalid));

        // TODO(xxx) do we update the version?

        // apply meta-data updates
        extent0->updateMetaData(blob_meta->meta_list);

        LOGTRACE << "Applied metadata update to extent 0 -- " << *extent0;
        err = persistCat->putMetaExtent(blob_meta->vol_id,
                                        blob_meta->blob_name, extent0);
    }

    if (!err.ok()) {
        LOGERROR << "Failed to update blob meta for " << *blob_meta;
    }
    return err;
}

//
// Updates committed blob in the Volume Catalog.
// Updates blob metadata and a given list of offset to object id mappings
// (not necessarily the whole blob).
//
Error
DmVolumeCatalog::putBlob(const BlobMetaDesc::const_ptr& blob_meta,
                         const BlobObjList::const_ptr& blob_obj_list,
                         const BlobTxId::const_ptr& tx_id)
{
    Error err(ERR_OK);
    std::vector<ObjectID> expunge_list;
    LOGTRACE << "Will commit blob " << *blob_meta << ";" << *blob_obj_list;

    // do not use this method if blob_obj_list is empty
    fds_verify(blob_obj_list->size() > 0);

    // retrieve extent0 and all extents that contain object info for
    // objects in 'blob_obj_list'

    // we need extent0 in any case, to update total blob size, etc
    BlobExtent0::ptr extent0 = persistCat->getMetaExtent(blob_meta->vol_id,
                                                         blob_meta->blob_name,
                                                         err);

    if (!err.ok() && (err != ERR_CAT_ENTRY_NOT_FOUND)) {
        LOGERROR << "Failed to retrieve extent 0 for " << *blob_meta;
        return err;
    }

    // apply meta-data updates
    extent0->updateMetaData(blob_meta->meta_list);

    // verify Vol Cat assumptions about obj sizes holds
    blob_obj_list->verify(extent0->maxObjSizeBytes());

    // iterate over all offsets, offsets are ordered, so once we
    // out of range for one extent, we will get next one
    BlobExtent::ptr extent = extent0;
    fds_extent_id extent_id = 0;   // we start with extent 0
    fds_uint64_t blob_size = extent0->blobSize();
    fds_uint64_t last_obj_size = extent0->lastObjSize();
    fds_uint64_t new_last_offset = blob_obj_list->lastOffset();
    for (BlobObjList::const_iter cit = blob_obj_list->cbegin();
         cit != blob_obj_list->cend();
         ++cit) {
        // if offset not in this extent, get extent containing this offset
        if (!extent->offsetInRange(cit->first)) {
            // we are done with previous extent, if this is not extent0
            // write extent to persistent storage (for now)
            if (extent_id > 0) {
                err = persistCat->putExtent(blob_meta->vol_id,
                                            blob_meta->blob_name,
                                            extent_id, extent);
                // TODO(xxx) if we fail writing extent, we may already
                // have written other extents of this blob
                // need to deal with partial extent updates
                fds_verify(err.ok());
            }
            // move to next extent
            extent_id = persistCat->getExtentId(blob_meta->vol_id,
                                                cit->first);
            extent = persistCat->getExtent(blob_meta->vol_id,
                                           blob_meta->blob_name,
                                           extent_id,
                                           err);

            if (!err.ok() && (err != ERR_CAT_ENTRY_NOT_FOUND)) {
                LOGERROR << "Failed to retrieve all extents for " << *blob_meta;
                return err;
            }
        }

        // if we are modifying existing offset, we may need to expunge the blob
        // (at the moment, until we implement proper GC)
        ObjectID old_obj;
        err = extent->getObjectInfo(cit->first, &old_obj);
        fds_verify(err.ok() || (err == ERR_NOT_FOUND));
        if (err.ok()) {
            // we will modify existing object, add to expunge list
            // TODO(xxx) check if null object id?
            expunge_list.push_back(old_obj);

            // if we are updating last offset, adjust blob size
            fds_verify(blob_size > 0);
            if (cit->first == extent0->lastOffset()) {
                // modifying existing last offset, update size
                blob_size -= last_obj_size;
                blob_size += (cit->second).size;
            } else if (cit->first == new_last_offset) {
                blob_size -= extent0->maxObjSizeBytes();
                blob_size += (cit->second).size;
            }
            // otherwise object sizes match
        } else {
            // new offset, update blob size
            blob_size += (cit->second).size;
        }

        // update offset in extent
        err = extent->updateOffset(cit->first, (cit->second).oid);
        fds_verify(err.ok());  // we already checked range
    }

    // actually update blob size and version
    extent0->setBlobSize(blob_size);
    extent0->incrementBlobVersion();

    // update last offset of the blob and trancate blob if needed
    if (blob_obj_list->endOfBlob()) {
        // we are changing the last offset of the blob, unless
        // these last offsets are the same
        if (new_last_offset < extent0->lastOffset()) {
            // truncating the blob!!!
            fds_verify(false);  // TODO(xxx) implement

        } else if (new_last_offset > extent0->lastOffset()) {
            // just update extent0 with new last offset
            extent0->setLastOffset(new_last_offset);
        }
    }

    if (extent_id > 0) {
        err = persistCat->putExtent(blob_meta->vol_id,
                                    blob_meta->blob_name,
                                    extent_id, extent);
        fds_verify(err.ok());  // TODO(xxx) partial write!
    }
    err = persistCat->putMetaExtent(blob_meta->vol_id,
                                    blob_meta->blob_name, extent0);
    fds_verify(err.ok() || (extent_id == 0));  // TODO(xxx) partial write!

    // TODO(xxx) call DM methods to expunge objects we have in the list

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
    LOGDEBUG << "Will retrieve blob meta for blob " << blob_name
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
