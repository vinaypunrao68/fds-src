/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <list>
#include <set>
#include <vector>
#include <string>
#include <dm-vol-cat/DmVolumeCatalog.h>
#include <dm-vol-cat/DmPersistVc.h>
#include <dm-vol-cat/DmVcCache.h>
#include <PerfTrace.h>

namespace fds {

DmVolumeCatalog::DmVolumeCatalog(char const *const name)
        : Module(name), expunge_cb(NULL)
{
    cacheCat = std::unique_ptr<DmCacheVolCatalog>(
        new DmCacheVolCatalog("DM Vol Cat Cache Layer"));

    persistCat = new DmPersistVolCatalog("DM Vol Cat Persistent Layer");
    // TODO(Andrew): The Module array should be able to take unique ptrs
    static Module* om_mods[] = {
        cacheCat.get(),
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

void DmVolumeCatalog::registerExpungeObjectsCb(expunge_objs_cb_t cb) {
    expunge_cb = cb;
}

BlobExtent0::ptr
DmVolumeCatalog::getMetaExtent(fds_volid_t volume_id,
                               const std::string& blob_name,
                               Error& error) {
    // Check cache for blob extent 0
    BlobExtent0::ptr extentZero = boost::dynamic_pointer_cast<BlobExtent0>(
        cacheCat->getExtent(volume_id,
                            blob_name,
                            0,
                            error));
    if (error != ERR_OK) {
        // Couldn't find the extent in cache, check persistent layer
        LOGDEBUG << "Did not find cache meta extent for " << blob_name
                 << ", checking persistent layer";

        // Read from persistent layer. The persistent layer will
        // create an entry if it didn't exist already.
        extentZero = persistCat->getMetaExtent(volume_id,
                                               blob_name,
                                               error);

        // Place the entry in the cache if we found a valid entry
        if (error != ERR_CAT_ENTRY_NOT_FOUND) {
            cacheCat->putExtent(volume_id, blob_name, extentZero);
        }
    } else {
        PerfTracer::incr(DM_CACHE_HIT, volume_id, blob_name);
    }

    return extentZero;
}

BlobExtent::ptr
DmVolumeCatalog::getExtent(fds_volid_t volume_id,
                           const std::string& blob_name,
                           fds_extent_id extent_id,
                           Error& error) {
    // TODO(Andrew): Combine the catalog and persistent layer
    // getExtent and getMetaExtent functions into one. Then
    // we can get ride of this check.
    fds_verify(extent_id != 0);

    // Check cache for blob extent
    BlobExtent::ptr extent = cacheCat->getExtent(volume_id,
                                                 blob_name,
                                                 extent_id,
                                                 error);
    if (error != ERR_OK) {
        // Couldn't find the extent in cache, check persistent layer
        LOGDEBUG << "Did not find cache meta extent for " << blob_name
                 << ", checking persistent layer";

        // Read from persistent layer. The persistent layer will
        // create an entry if it didn't exist already.
        extent = persistCat->getExtent(volume_id,
                                       blob_name,
                                       extent_id,
                                       error);

        // Place the entry in the cache if we found a valid entry
        if (error != ERR_CAT_ENTRY_NOT_FOUND) {
            cacheCat->putExtent(volume_id, blob_name, extent);
        }
    } else {
        PerfTracer::incr(DM_CACHE_HIT, volume_id, blob_name);
    }

    return extent;
}

Error DmVolumeCatalog::putMetaExtent(fds_volid_t volume_id,
                                     const std::string& blob_name,
                                     const BlobExtent0::const_ptr& meta_extent) {
    // Persist the meta extent
    Error err = persistCat->putMetaExtent(volume_id, blob_name, meta_extent);

    // Drop a copy in the cache
    if (err == ERR_OK) {
        err = cacheCat->putExtent(volume_id, blob_name, meta_extent);
    }

    return err;
}

Error DmVolumeCatalog::putExtents(fds_volid_t volume_id,
                                  const std::string& blob_name,
                                  const BlobExtent0::const_ptr& meta_extent,
                                  const std::vector<BlobExtent::const_ptr>& extents) {
    // Persist the extents
    Error err = persistCat->putExtents(volume_id, blob_name, meta_extent, extents);

    // Drop a copy in the cache
    if (err == ERR_OK) {
        err = cacheCat->putExtents(volume_id, blob_name, meta_extent, extents);
    }

    return err;
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

    // create volume catalog in VC persistent layer
    err = persistCat->createCatalog(voldesc);

    if (err == ERR_OK) {
        // Create volume catalog cache
        err = cacheCat->createCache(voldesc);
    }
    return err;
}

//
// copy volume for  snapshot/clone
//
Error DmVolumeCatalog::copyVolume(VolumeDesc & voldesc) {
    LOGDEBUG << "Creating a snapshot '" << voldesc.name << "' of a volume '"
            << voldesc.volUUID << "'"  << "srcVolume: "<< voldesc.srcVolumeId;

    // create a snapshot catalog
    Error rc = persistCat->copyVolume(voldesc.srcVolumeId, voldesc.volUUID, voldesc.isSnapshot(),
                       voldesc.isSnapshot());
    if (rc.ok()) {
        // Create cache for a snapshot
        rc = cacheCat->createCache(voldesc);
    }
    return rc;
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

    // The cache has been created already...warm?
    // initialized VC in persistent layer
    err = persistCat->openCatalog(volume_id);
    return err;
}

//
// Sync catalog of volume 'volume_id'
//
Error DmVolumeCatalog::syncCatalog(fds_volid_t volume_id,
                                   const NodeUuid& dm_uuid) {
    // TODO(xxx) when we have cache, flush data first???
    return persistCat->syncCatalog(volume_id, dm_uuid);
}

//
// Deletes each blob in the volume 'volume_id' and marks volume as deleted
//
Error DmVolumeCatalog::removeVolumeMeta(fds_volid_t volume_id)
{
    Error err(ERR_OK);
    LOGDEBUG << "Will remove volume meta for volune " << std::hex
             << volume_id << std::dec;

    // Delete the volume's catalog cache
    cacheCat->removeCache(volume_id);

    // TODO(xxx) implement
    return err;
}

//
// Returns true if the volume does not contain any valid blobs.
// A valid blob is a non-deleted blob version.
//
Error DmVolumeCatalog::markVolumeDeleted(fds_volid_t volume_id)
{
    // TODO(Andrew): For now, ignore the cache since it never holds
    // any dirty data (i.e., nothing that's not already in the persistent
    // catalog). Once we add dirty entries, we need to revisit.

    return persistCat->markVolumeDeleted(volume_id);
}

Error DmVolumeCatalog::deleteEmptyCatalog(fds_volid_t volume_id) {
    Error err = persistCat->deleteEmptyCatalog(volume_id);
    if (err.ok()) {
        cacheCat->removeCache(volume_id);
    }
    LOGNOTIFY << "Removed catalog and cache for volume " << std::hex
              << volume_id << std::dec << " err " << err;
    return err;
}

Error DmVolumeCatalog::getVolumeMeta(fds_volid_t volume_id,
                                     fds_uint64_t* size,
                                     fds_uint64_t* blob_count,
                                     fds_uint64_t* object_count) {
    Error err(ERR_OK);
    {
        SCOPEDREAD(lockVolDetailsMap_);
        DmVolumeDetailsMap_t::const_iterator iter = volDetailsMap_.find(volume_id);
        if (volDetailsMap_.end() != iter) {
            *size = iter->second->size;
            *blob_count = iter->second->blobCount;
            *object_count = iter->second->objectCount;
            return err;
        }
    }

    err = getVolumeMetaInternal(volume_id, size, blob_count, object_count);

    SCOPEDWRITE(lockVolDetailsMap_);
    volDetailsMap_[volume_id].reset(new DmVolumeDetails(volume_id, *size,
            *blob_count, *object_count));

    return err;
}

Error DmVolumeCatalog::getVolumeMetaInternal(fds_volid_t volume_id,
                                     fds_uint64_t* size,
                                     fds_uint64_t* blob_count,
                                     fds_uint64_t* object_count) {
    Error err(ERR_OK);
    std::vector<BlobExtent0Desc> desc_list;
    std::vector<BlobExtent0Desc>::const_iterator cit;
    fds_uint64_t volume_size = 0;

    // TODO(Andrew): Go directly to the persistent catalog since we have
    // to iterate it anyways. No point in going through the cache until
    // we have the possibility of dirty entries.
    // retrieve list of extent 0
    err = persistCat->getMetaDescList(volume_id, desc_list);
    if (!err.ok()) {
        LOGERROR << "Failed to retrieve volume meta for volume " << std::hex
                 << volume_id << std::hex << " " << err;
        return err;
    }

    // calculate size of volume
    for (cit = desc_list.cbegin(); cit != desc_list.cend(); ++cit) {
        volume_size += (*cit).blob_size;

        // get object count for the blob
        // get extent 0
        BlobExtent0::ptr extent0 = getMetaExtent(volume_id, cit->blob_name, err);
        if (!err.ok()) {
            LOGERROR << "Failed to retrieve extent0 for blob '" << cit->blob_name << "'";
            return err;
        }
        fds_verify(extent0->blobVersion() != blob_version_invalid);
        *object_count += extent0->objectCount();

        if (extent0->blobSize() > 0) {
            fds_extent_id last_extent = persistCat->getExtentId(volume_id,
                    extent0->lastBlobOffset());
            for (fds_extent_id ext_id = 1; ext_id <= last_extent; ++ext_id) {
                // get next extent
                BlobExtent::ptr extent = getExtent(volume_id, cit->blob_name, ext_id, err);
                if (err.ok()) {
                    *object_count += extent->objectCount();
                }
            }
        }
    }

    // return size and blob count
    *blob_count = desc_list.size();
    *size = volume_size;

    LOGDEBUG << "Volume " << std::hex << volume_id << std::dec
             << " size " << volume_size << " blobs " << *blob_count;

    return err;
}

//
// Updates committed blob in the Volume Catalog.
// Updates blob exluding list of offset to object id mappings.
//
Error
DmVolumeCatalog::putBlobMeta(fds_volid_t volume_id,
                             const std::string& blob_name,
                             const MetaDataList::const_ptr& meta_list,
                             const BlobTxId::const_ptr& tx_id)
{
    Error err(ERR_OK);
    LOGDEBUG << "Will commit meta for " << std::hex << volume_id
             << std::dec << "," << blob_name << " " << *meta_list;

    // blob meta is in extent 0, so retrieve extent 0
    BlobExtent0::ptr extent0 = getMetaExtent(volume_id,
                                             blob_name,
                                             err);

    // even if not found, we get an allocated extent0 which we will
    // fill in
    if (err.ok() || (err == ERR_CAT_ENTRY_NOT_FOUND)) {
        LOGDEBUG << "Retrieved extent 0 for volume " << std::hex
                 << volume_id << std::dec << " " << *extent0
                 << " error " << err;

        fds_verify((err == ERR_CAT_ENTRY_NOT_FOUND) ||
                   (extent0->blobVersion() != blob_version_invalid));

        // apply meta-data updates
        extent0->updateMetaData(meta_list);

        // update version
        extent0->incrementBlobVersion();

        LOGDEBUG << "Applied metadata update to extent 0 -- " << *extent0;
        err = putMetaExtent(volume_id, blob_name, extent0);
    }

    if (!err.ok()) {
        LOGERROR << "Failed to update blob meta for " << std::hex << volume_id
                 << std::dec << "," << blob_name << " " << *meta_list;
    }
    return err;
}

//
// Updates committed blob in the Volume Catalog.
// Updates blob metadata and a given list of offset to object id mappings
// (not necessarily the whole blob).
//
Error
DmVolumeCatalog::putBlob(fds_volid_t volume_id,
                         const std::string& blob_name,
                         const MetaDataList::const_ptr& meta_list,
                         const BlobObjList::const_ptr& blob_obj_list,
                         const BlobTxId::const_ptr& tx_id)
{
    Error err(ERR_OK);
    std::vector<BlobExtent::const_ptr> extent_list;
    std::vector<ObjectID> expunge_list;
    LOGDEBUG << "Will commit blob " << blob_name
             << "; " << *blob_obj_list;

    // do not use this method if blob_obj_list is empty
    fds_verify(blob_obj_list->size() > 0);

    // retrieve extent0 and all extents that contain object info for
    // objects in 'blob_obj_list'

    // we need extent0 in any case, to update total blob size, etc
    BlobExtent0::ptr extent0 = getMetaExtent(volume_id,
                                             blob_name,
                                             err);

    bool new_blob = false;
    if (!err.ok()) {
        if (err != ERR_CAT_ENTRY_NOT_FOUND) {
            LOGERROR << "Failed to retrieve extent 0 for " << std::hex
                     << volume_id << std::dec << "," << blob_name;
            return err;
        } else {
            new_blob = true;
        }
    }

    // apply meta-data updates
    if (meta_list) {
        extent0->updateMetaData(meta_list);
    }

    // verify Vol Cat assumptions about obj sizes holds
    blob_obj_list->verify(extent0->maxObjSizeBytes());

    // iterate over all offsets, offsets are ordered, so once we
    // out of range for one extent, we will get next one
    BlobExtent::ptr extent = extent0;
    fds_extent_id extent_id = 0;   // we start with extent 0
    fds_uint64_t blob_size = extent0->blobSize();
    fds_uint64_t old_blob_size = blob_size;
    fds_uint64_t last_obj_size = extent0->lastObjSize();
    fds_uint64_t new_last_offset = blob_obj_list->lastOffset();
    for (BlobObjList::const_iter cit = blob_obj_list->cbegin();
         cit != blob_obj_list->cend();
         ++cit) {
        // if offset not in this extent, get extent containing this offset
        if (!extent->offsetInRange(cit->first)) {
            // we are done with previous extent, if this is not extent0
            // add to extent list. We will persist all extents that we
            // updates as one atomic operation at the end
            if (extent_id > 0) {
                extent_list.push_back(extent);
            }
            extent_id = persistCat->getExtentId(volume_id, cit->first);
            extent = getExtent(volume_id, blob_name, extent_id, err);
            if (!err.ok() && (err != ERR_CAT_ENTRY_NOT_FOUND)) {
                LOGERROR << "Failed to retrieve all extents for " << blob_name
                         << " in vol " << std::hex << volume_id << std::dec << " " << err;
                return err;  // we did not make any changes to the DB
            }
        }

        // if we are modifying existing offset, we may need to expunge the blob
        // (at the moment, until we implement proper GC)
        ObjectID old_obj;
        err = extent->getObjectInfo(cit->first, &old_obj);
        fds_verify(err.ok() || (err == ERR_NOT_FOUND));
        if (err.ok()) {
            // we will modify existing object, add to expunge list
            if (old_obj != NullObjectID) {
                // null obj does not physically exist
                expunge_list.push_back(old_obj);
            }

            // if we are updating last offset, adjust blob size
            if (cit->first == extent0->lastBlobOffset()) {
                // modifying existing last offset, update size
                if (old_obj != NullObjectID) {
                    fds_verify(blob_size >= last_obj_size);
                    blob_size -= last_obj_size;
                }
                blob_size += (cit->second).size;
            } else if (cit->first == new_last_offset) {
                fds_verify(old_obj != NullObjectID);  // is this possible?
                fds_verify(blob_size >= extent0->maxObjSizeBytes());
                blob_size -= extent0->maxObjSizeBytes();
                blob_size += (cit->second).size;
            }
            // otherwise object sizes match
        } else {
            // new offset, update blob size
            blob_size += (cit->second).size;

            SCOPEDWRITE(lockVolDetailsMap_);
            DmVolumeDetailsMap_t::iterator iter = volDetailsMap_.find(volume_id);
            if (volDetailsMap_.end() != iter) {
                iter->second->objectCount.fetch_add(1);
            }
        }

        // update offset in extent
        err = extent->updateOffset(cit->first, (cit->second).oid);
        fds_verify(err.ok());  // we already checked range
    }

    // update last offset of the blob and truncate blob if needed
    if ((extent0->blobSize() == 0) ||
        (new_last_offset > extent0->lastBlobOffset())) {
        // increasing blob size, set last offset
        LOGDEBUG << "Updating last blob offset for blob " << blob_name
                 << " to" << new_last_offset;
        extent0->setLastBlobOffset(new_last_offset);
    } else if ((blob_obj_list->endOfBlob()) &&
               (new_last_offset < extent0->lastBlobOffset())) {
        // truncating the blob!!!
        fds_uint32_t num_objs_rm = 0;
        fds_uint64_t trunc_size = 0;
        fds_uint64_t last_trunc_offset = new_last_offset;
        fds_bool_t done = false;

        while (!done) {
            num_objs_rm = extent->truncate(last_trunc_offset, &expunge_list);
            if (extent0->lastBlobOffset() > extent->lastOffsetInRange()) {
                // more objects to expunge
                trunc_size = num_objs_rm * extent0->maxObjSizeBytes();
            } else {
                // that was last extent
                fds_verify(num_objs_rm > 0);
                trunc_size = last_obj_size + (num_objs_rm - 1) * extent0->maxObjSizeBytes();
                done = true;
            }
            fds_verify(blob_size >= trunc_size);
            blob_size -= trunc_size;
            if (!done) {
                last_trunc_offset = extent->lastOffsetInRange();
                if (extent_id > 0) {
                    extent_list.push_back(extent);
                }
                extent_id = persistCat->getExtentId(volume_id,
                                                    last_trunc_offset + extent0->maxObjSizeBytes());
                extent = getExtent(volume_id, blob_name, extent_id, err);
                // even if we have gaps between extents and getExtent returns empty extent
                // this will result in delete op for this extent, which is ok
                if (!err.ok() && (err != ERR_CAT_ENTRY_NOT_FOUND)) {
                    LOGERROR << "Failed to retrieve all extents for " << blob_name
                             << " in vol " << std::hex << volume_id << std::dec << " " << err;
                    return err;  // we did not make any changes to the DB yet
                }
            }

            if (num_objs_rm > 0) {
                SCOPEDWRITE(lockVolDetailsMap_);
                DmVolumeDetailsMap_t::iterator iter = volDetailsMap_.find(volume_id);
                if (volDetailsMap_.end() != iter) {
                    iter->second->objectCount.fetch_sub(num_objs_rm);
                }
            }
        }
        extent0->setLastBlobOffset(new_last_offset);
    }

    // actually update blob size and version
    extent0->setBlobSize(blob_size);
    extent0->incrementBlobVersion();

    {
        SCOPEDWRITE(lockVolDetailsMap_);
        DmVolumeDetailsMap_t::iterator iter = volDetailsMap_.find(volume_id);
        if (volDetailsMap_.end() != iter) {
            fds_uint64_t diff = 0;
            if (blob_size >= old_blob_size) {
                diff = blob_size - old_blob_size;
                iter->second->size.fetch_add(diff);
            } else {
                diff = old_blob_size - blob_size;
                iter->second->size.fetch_sub(diff);
            }
            if (new_blob) {
                iter->second->blobCount.fetch_add(1);
            }
        }
    }

    if (extent_id > 0) {
        extent_list.push_back(extent);
    }
    err = putExtents(volume_id, blob_name, extent0, extent_list);
    if (!err.ok()) {
        LOGERROR << "Failed to write update to persistent volume catalog for "
                 << std::hex << volume_id << std::dec << "," << blob_name
                 << " " << err;
        return err;
    }

    // actually expunge objects that were dereferenced by the blob
    // TODO(xxx) later that should become part of GC and done in background
    fds_verify(expunge_cb);
    err = expunge_cb(volume_id, expunge_list);

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
// Returns list of blobs in the volume 'volume_id'
//
Error DmVolumeCatalog::listBlobs(fds_volid_t volume_id,
                                 fpi::BlobInfoListType* bmeta_list)
{
    Error err(ERR_OK);
    std::vector<BlobExtent0Desc> desc_list;
    std::vector<BlobExtent0Desc>::const_iterator cit;

    LOGDEBUG << "Will retrieve list of blobs for volume "
             << std::hex << volume_id << std::dec;

    // Query directly from the persistent catalog, since the
    // cache is unlikely to have everything anyways.
    err = persistCat->getMetaDescList(volume_id, desc_list);
    if (err.ok()) {
        for (cit = desc_list.cbegin(); cit != desc_list.cend(); ++cit) {
            fpi::FDSP_BlobInfoType binfo;
            binfo.blob_name = (*cit).blob_name;
            binfo.blob_size = (*cit).blob_size;
            (*bmeta_list).push_back(binfo);
        }
    }

    return err;
}

//
// Retrieves metadata part of the blob
//
Error DmVolumeCatalog::getBlobMeta(fds_volid_t volume_id,
                                   const std::string& blob_name,
                                   blob_version_t* blob_version,
                                   fds_uint64_t* blob_size,
                                   fpi::FDSP_MetaDataList* meta_list)
{
    Error err(ERR_OK);
    LOGDEBUG << "Will retrieve blob meta for blob " << blob_name
             << " volid " << std::hex << volume_id << std::dec;

    // blob meta is in extent 0
    BlobExtent0::ptr extent0 = getMetaExtent(volume_id,
                                             blob_name,
                                             err);

    if (err.ok()) {
        // Return not found if blob is marked for deleteion
        if (extent0->isDeleted() == true) {
            err = ERR_CAT_ENTRY_NOT_FOUND;
        } else {
            // we got blob meta, fill in version, size, and meta list
            *blob_version = extent0->blobVersion();
            *blob_size = extent0->blobSize();
            extent0->toMetaFdspPayload(*meta_list);
        }
    }

    return err;
}

//
// Retrieves info for the whole blob (all extents)
//
Error DmVolumeCatalog::getBlob(fds_volid_t volume_id,
                               const std::string& blob_name,
                               blob_version_t* blob_version,
                               fpi::FDSP_MetaDataList* meta_list,
                               fpi::FDSP_BlobObjectList* obj_list)
{
    Error err(ERR_OK);
    LOGDEBUG << "Will retrieve blob " << blob_name << " volid "
             << std::hex << volume_id << std::dec;

    // get extent 0
    BlobExtent0::ptr extent0 = getMetaExtent(volume_id,
                                             blob_name,
                                             err);
    if (!err.ok()) {
        LOGNOTIFY << "Failed to retrieve blob " << blob_name
                  << " volid " << std::hex << volume_id << std::dec
                  << " err " << err;
        return err;
    }

    // we got blob meta, fill in version, size, and meta list
    *blob_version = extent0->blobVersion();
    extent0->toMetaFdspPayload(*meta_list);

    // find out number of extents we need to read to get the whole
    // blob, and read all the extents to get all objects
    BlobExtent::ptr extent = extent0;
    fds_extent_id last_extent = 0;
    if (extent0->blobSize() > 0) {
        last_extent = persistCat->getExtentId(volume_id,
                                              extent0->lastBlobOffset());
    }
    fds_uint64_t last_obj_sz = extent0->lastObjSize();
    extent->addToFdspPayload(*obj_list,
                             extent0->lastBlobOffset(), last_obj_sz);
    for (fds_extent_id ext_id = 1; ext_id <= last_extent; ++ext_id) {
        // get next extent
        extent = getExtent(volume_id, blob_name, ext_id, err);
        // note that even if extent not in db, we get empty extent and
        // addToFdspPayload method will not add any objs to the list
        if (!err.ok() && (err != ERR_CAT_ENTRY_NOT_FOUND)) {
            LOGERROR << "Failed to retrieve all extents to retrieve blob "
                     << blob_name << " volid " << std::hex << volume_id
                     << std::dec << " err " << err;
            return err;
        }
        // note that even if extent not in db, we get empty extent and
        // addToFdspPayload method will not add any objs to the list
        extent->addToFdspPayload(*obj_list, extent0->lastBlobOffset(), last_obj_sz);
    }

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
    std::vector<ObjectID> expunge_list;
    LOGDEBUG << "Will delete blob " << blob_name << " volid " << std::hex
             << volume_id << std::dec << " ver " << blob_version;

    // get extent 0
    BlobExtent0::ptr extent0 = getMetaExtent(volume_id,
                                             blob_name,
                                             err);
    if (err == ERR_CAT_ENTRY_NOT_FOUND) {
        LOGWARN << "No blob found with name " << blob_name << " for vol "
                << std::hex << volume_id << std::dec << ", so nothing to delete";
        err = ERR_BLOB_NOT_FOUND;
        return err;
    } else if (!err.ok()) {
        LOGERROR << "Failed to retrieve extent0 for blob " << blob_name;
        return err;
    }

    LOGDEBUG << "Located existing blob " << blob_name << " vol " << std::hex
             << volume_id << std::dec << " with version " << extent0->blobVersion()
             << " size " << extent0->blobSize();
    fds_verify(extent0->blobVersion() != blob_version_invalid);

    // If the current version is a delete marker, it's
    // already deleted so return not found.
    if (extent0->blobVersion() == blob_version_deleted) {
        err = ERR_BLOB_NOT_FOUND;
        return err;
    }

    // get the last extent of this blob
    fds_extent_id last_extent = 0;
    if (extent0->blobSize() > 0) {
        last_extent = persistCat->getExtentId(volume_id,
                                              extent0->lastBlobOffset());
    }

    // update/delete extent 0 first
    if (extent0->blobSize() > 0) {
        {
            // Decrement the cached volume details
            SCOPEDWRITE(lockVolDetailsMap_);
            DmVolumeDetailsMap_t::iterator iter = volDetailsMap_.find(volume_id);
            if (volDetailsMap_.end() != iter) {
                iter->second->blobCount.fetch_sub(1);
                iter->second->size.fetch_sub(extent0->blobSize());
                iter->second->objectCount.fetch_sub(extent0->objectCount());
            }
        }

        extent0->deleteAllObjects(&expunge_list);  // get objs to expunge
    }

    if (blob_version == extent0->blobVersion()) {
        // Remove extent 0 from the cache
        err = cacheCat->removeExtent(volume_id, blob_name, 0);
        fds_verify(err == ERR_OK);
        // delete persistent extent 0
        err = persistCat->deleteExtent(volume_id, blob_name, 0);
    } else if (blob_version == blob_version_invalid) {
        // clear meta list, set blob size to 0 and set version deleted
        // and write back to persist catalog -- a delete marker
        extent0->markDeleted();
        LOGDEBUG << "Writing extent 0 delete marker for blob " << blob_name;
        err = putMetaExtent(volume_id, blob_name, extent0);
    }
    if (!err.ok()) {
        // if we failed to delete/update extent 0, we can still return error
        // -- extent is not changed in persistent vol cat
        LOGERROR << "Failed to delete blob " << blob_name << " vol cat stil "
                 << "has blob unchanged";
        return err;
    }

    // delete all remaining extents from persistent vol cat
    for (fds_extent_id ext_id = 1; ext_id <= last_extent; ++ext_id) {
        // we read extents so we know which objects to expunge
        // TODO(xxx) we can probably do that in background later, but
        // make sure that if blob written again, we don't have extents
        // from the prev version still in persistent vol cat !
        BlobExtent::ptr extent =
                persistCat->getExtent(volume_id, blob_name, ext_id, err);
        if (err.ok()) {
            {
                SCOPEDWRITE(lockVolDetailsMap_);
                DmVolumeDetailsMap_t::iterator iter = volDetailsMap_.find(volume_id);
                if (volDetailsMap_.end() != iter) {
                    iter->second->objectCount.fetch_sub(extent->objectCount());
                }
            }
            extent->deleteAllObjects(&expunge_list);
            // Remove extent 0 from the cache
            err = cacheCat->removeExtent(volume_id, blob_name, 0);
            fds_verify(err == ERR_OK);
            err = persistCat->deleteExtent(volume_id, blob_name, ext_id);
        }
        fds_verify(err.ok() || (err == ERR_CAT_ENTRY_NOT_FOUND));
    }

    fds_verify(expunge_cb);
    err = expunge_cb(volume_id, expunge_list);
    return err;
}

}  // namespace fds
