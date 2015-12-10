/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <algorithm>
#include <string>
#include <set>
#include <vector>

#include <fds_module.h>
#include <fds_process.h>

#include <DmBlobTypes.h>

#include <dm-vol-cat/DmPersistVolDB.h>
#include <dm-vol-cat/DmPersistVolFile.h>
#include <dm-vol-cat/DmVolumeCatalog.h>

#define ENSURE_SEQUENCE_ADV(seq_a, seq_b, volId, blobName) \
        auto const seq_ev_a = (seq_a); auto const seq_ev_b = (seq_b); \
        if (seq_ev_a >= seq_ev_b) { \
            LOGERROR << "Rejecting request to overwrite blob with older sequence id on vol:" \
                     << (volId) << " blob: " << (blobName) << " old seq_id: " \
                     << seq_ev_a << " new seq_id: "<< seq_ev_b; \
            return ERR_BLOB_SEQUENCE_ID_REGRESSION; \
        }

#define GET_VOL(volId) \
        DmPersistVolCat::ptr vol; \
        synchronized(volMapLock_) { \
            std::unordered_map<fds_volid_t, DmPersistVolCat::ptr>::iterator iter = \
                    volMap_.find(volId); \
            if (volMap_.end() != iter) { \
                vol = iter->second; \
            } \
        }

#define GET_VOL_N_CHECK_DELETED(volId) \
        GET_VOL(volId); \
        if (!vol || vol->isMarkedDeleted()) { \
            LOGWARN << "Volume " << std::hex << volId << std::dec << " does not exist"; \
            return ERR_VOL_NOT_FOUND; \
        }

#define HANDLE_VOL_NOT_ACTIVATED() \
    if (!vol->isActivated()) { \
        LOGWARN << "Volume " << std::hex << volId << std::dec << " is not activated"; \
        return ERR_DM_VOL_NOT_ACTIVATED; \
    }

namespace fds {

DmVolumeCatalog::DmVolumeCatalog(char const * const name) : Module(name), expungeCb_(0) {}

DmVolumeCatalog::~DmVolumeCatalog() {}

Error DmVolumeCatalog::getBlobMetaDesc(fds_volid_t volId, const std::string & blobName,
            BlobMetaDesc & blobMeta) {
    GET_VOL_N_CHECK_DELETED(volId);
    HANDLE_VOL_NOT_ACTIVATED();

    Error rc = vol->getBlobMetaDesc(blobName, blobMeta);
    if (rc.ok()) {
        // Return not found if blob is marked deleted
        if (blob_version_invalid == blobMeta.desc.version) {
            rc = ERR_CAT_ENTRY_NOT_FOUND;
        }
    }
    return rc;
}

Error DmVolumeCatalog::addCatalog(const VolumeDesc & voldesc) {
    LOGDEBUG << "Will add catalog for volume: '" << voldesc.name << "' id: '" <<
            std::hex << voldesc.volUUID << std::dec << "'";

    DmPersistVolCat::ptr vol;
    /*
     * TODO(umesh): commented out for beta for commit log and consistent hot snapshot features
    if (fpi::FDSP_VOL_S3_TYPE == voldesc.volType) {
    */
        vol.reset(new DmPersistVolDB(voldesc.volUUID, voldesc.maxObjSizeInBytes,
                    voldesc.isSnapshot(), voldesc.isSnapshot(), voldesc.isClone(),
                    voldesc.isSnapshot() ? voldesc.srcVolumeId : invalid_vol_id));
    /*
    } else {
        vol.reset(new DmPersistVolFile(voldesc.volUUID, voldesc.maxObjSizeInBytes,
                    voldesc.isSnapshot(), voldesc.isSnapshot(),
                    voldesc.isSnapshot() ? voldesc.srcVolumeId : invalid_vol_id));
    }
    */

    FDSGUARD(volMapLock_);

    volMap_[voldesc.volUUID] = vol;

    return ERR_OK;
}

Error DmVolumeCatalog::copyVolume(const VolumeDesc & voldesc) {
    LOGDEBUG << "Creating a copy '" << voldesc.name << "' id: '" << voldesc.volUUID
            << "' of a volume '" << voldesc.srcVolumeId << "'";

    fds_uint32_t objSize = 0;
    fpi::FDSP_VolType volType = fpi::FDSP_VOL_S3_TYPE;
    synchronized(volMapLock_) {
        std::unordered_map<fds_volid_t, DmPersistVolCat::ptr>::const_iterator iter =
                volMap_.find(voldesc.srcVolumeId);
        fds_verify(volMap_.end() != iter);
        objSize = iter->second->getObjSize();
        volType = iter->second->getVolType();
    }

    const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
    std::ostringstream oss;
    oss << root->dir_sys_repo_dm() << voldesc.srcVolumeId << "/" << voldesc.srcVolumeId
            << "_vcat.ldb";
    std::string dbDir = oss.str();

    oss.clear();
    oss.str("");
    if (!voldesc.isSnapshot()) {
        oss << root->dir_sys_repo_dm() <<  voldesc.volUUID;
    } else {
        oss << root->dir_user_repo_dm() << voldesc.srcVolumeId << "/snapshot";
    }

    FdsRootDir::fds_mkdir(oss.str().c_str());

    oss << "/" << voldesc.volUUID << "_vcat.ldb";
    std::string copyDir =  oss.str();

    DmPersistVolCat::ptr voldir;
    synchronized(volMapLock_) {
        voldir = volMap_[voldesc.srcVolumeId];
    }
    Error rc = voldir->copyVolDir(copyDir);

    if (rc.ok()) {
        DmPersistVolCat::ptr vol;
        /*
         * TODO(umesh): commented out for beta for commit log and consistent hot snapshot features
        if (fpi::FDSP_VOL_S3_TYPE == volType) {
        */
            vol.reset(new DmPersistVolDB(voldesc.volUUID, objSize, voldesc.isSnapshot(),
                    voldesc.isSnapshot(), voldesc.isClone(), voldesc.srcVolumeId));
        /*
        } else {
            vol.reset(new DmPersistVolFile(voldesc.volUUID, objSize, voldesc.isSnapshot(),
                    voldesc.isSnapshot(), voldesc.srcVolumeId));
        }
        */

        FDSGUARD(volMapLock_);
        volMap_[voldesc.volUUID] = vol;
    }

    return rc;
}

Error DmVolumeCatalog::activateCatalog(fds_volid_t volId) {
    LOGDEBUG << "Will activate catalog for volume " << std::hex << volId << std::dec;

    GET_VOL_N_CHECK_DELETED(volId);

    Error rc = vol->activate();
    if (!rc.ok()) {
        LOGERROR << "Failed to activate catalog for volume " << std::hex << volId << std::dec
                << ", error: " << rc;
    }

    return rc;
}

Error DmVolumeCatalog::reloadCatalog(const VolumeDesc & voldesc) {
    LOGDEBUG << "Will reload catalog for volume " << std::hex << voldesc.volUUID << std::dec;
    deleteEmptyCatalog(voldesc.volUUID, false);
    Error rc = addCatalog(voldesc);
    if (!rc.ok()) {
        LOGWARN << "Failed to re-instantiate the volume '" << std::hex << voldesc.volUUID
                << std::dec << "'";
        return rc;
    }
    return activateCatalog(voldesc.volUUID);
}

void DmVolumeCatalog::registerExpungeObjectsCb(expunge_objs_cb_t cb) {
    expungeCb_ = cb;
}

Error DmVolumeCatalog::markVolumeDeleted(fds_volid_t volId) {
    LOGDEBUG << "Will mark volume '" << std::hex << volId << std::dec << "' as deleted";

    GET_VOL(volId);
    if (!vol) {
        LOGWARN << "Volume " << std::hex << volId << std::dec << " does not exist";
        return ERR_VOL_NOT_FOUND;
    }

    Error rc = vol->markDeleted();
    if (!rc.ok()) {
        LOGERROR << "Failed to mark volume '" << std::hex << volId << std::dec <<
                "' as deleted. error: " << rc;
    }
    return rc;
}

Error DmVolumeCatalog::deleteEmptyCatalog(fds_volid_t volId, bool checkDeleted /* = true */) {
    LOGDEBUG << "Will delete catalog for volume '" << std::hex << volId << std::dec << "'";
    GET_VOL(volId);
    if (!vol) return ERR_VOL_NOT_FOUND;


        // wait for all snapshots to be released
    if (0 != vol->getNumInMemorySnapshots()) {
        LOGWARN << "waiting for all db mem snapshots to be released.";
        uint count = 0;
        uint sleeptime = 500000;  // half a second
        uint maxwait = 10*60;  // 10 mins

        do {
            if (count && count % 40 == 0) {
                LOGWARN << "still waiting for db snaps to be released "
                        << " for vol:" << volId
                        << " waittime: " << (count/2) << " seconds";
            }
            usleep(500000);
            ++count;
        } while ( count < (maxwait*2) && 0 != vol->getNumInMemorySnapshots());

        if (0 != vol->getNumInMemorySnapshots()) {
            LOGCRITICAL << "Mem snaps not released even after "
                        << (count/2) << " seconds"
                        << " for vol:" << volId
                        << " .. please check";
        }
    }

    synchronized(volMapLock_) {
        std::unordered_map<fds_volid_t, DmPersistVolCat::ptr>::iterator iter =
                volMap_.find(volId);
        if (volMap_.end() != iter && (!checkDeleted ||
                                      iter->second->isMarkedDeleted() ||
                                      iter->second->isSnapshot()
                                      )) {
            volMap_.erase(iter);
        }
    }

    return ERR_OK;
}

Error DmVolumeCatalog::statVolume(fds_volid_t volId,
                                  fds_uint64_t* volSize,
                                  fds_uint64_t* blobCount,
                                  fds_uint64_t* objCount) {
    Error rc(ERR_OK);
    synchronized(lockVolSummaryMap_) {
        DmVolumeSummaryMap_t::const_iterator iter = volSummaryMap_.find(volId);
        if (volSummaryMap_.end() != iter) {
            *volSize = iter->second->size;
            *blobCount = iter->second->blobCount;
            *objCount = iter->second->objectCount;
            return rc;
        }
    }

    rc = statVolumeInternal(volId, volSize, blobCount, objCount);

    if (rc.ok()) {
        FDSGUARD(lockVolSummaryMap_);
        DmVolumeSummaryMap_t::const_iterator iter = volSummaryMap_.find(volId);
        if (volSummaryMap_.end() == iter) {
            volSummaryMap_[volId].reset(new DmVolumeSummary(volId, *volSize,
                    *blobCount, *objCount));
        }
    }

    return rc;
}

Error DmVolumeCatalog::statVolumeInternal(fds_volid_t volId, fds_uint64_t * volSize,
            fds_uint64_t * blobCount, fds_uint64_t * objCount) {
    GET_VOL_N_CHECK_DELETED(volId);
    HANDLE_VOL_NOT_ACTIVATED();

    std::vector<BlobMetaDesc> blobMetaList;
    Error rc = vol->getAllBlobMetaDesc(blobMetaList);
    if (!rc.ok()) {
        LOGERROR << "Failed to retrieve volume status for volume: '" << std::hex
                << volId << std::dec << "' error: '" << rc << "'";
        return rc;
    }

    *blobCount = blobMetaList.size();
    // calculate size of volume
    for (const auto & it : blobMetaList) {
        *volSize += it.desc.blob_size;
        *objCount += it.desc.blob_size / vol->getObjSize();

        if (it.desc.blob_size % vol->getObjSize()) {
            *objCount += 1;
        }
    }

    LOGDEBUG << "Volume: '" << std::hex << volId << std::dec << "' size: '" << *volSize
            << "' blobs: '" << *blobCount << "' objects: '" << *objCount << "'";
    return rc;
}

Error DmVolumeCatalog::setVolumeMetadata(fds_volid_t volId,
                                         const fpi::FDSP_MetaDataList &metadataList,
                                         const sequence_id_t seq_id) {
    GET_VOL_N_CHECK_DELETED(volId);
    HANDLE_VOL_NOT_ACTIVATED();

    VolumeMetaDesc volMetaDesc(metadataList, seq_id);
    return vol->putVolumeMetaDesc(volMetaDesc);
}

Error DmVolumeCatalog::getVolumeMetadata(fds_volid_t volId,
                                         fpi::FDSP_MetaDataList &metadataList) {
    GET_VOL_N_CHECK_DELETED(volId);
    HANDLE_VOL_NOT_ACTIVATED();

    VolumeMetaDesc volMetaDesc(metadataList, 0);
    Error rc = vol->getVolumeMetaDesc(volMetaDesc);
    if (!rc.ok()) {
        LOGERROR << "Unable to get metadata for volume " << volId << ", " << rc;
        return rc;
    }

    // Put key-value pairs into type to return
    fpi::FDSP_MetaDataPair metadataPair;
    for (const auto & it : volMetaDesc.meta_list) {
        metadataPair.key   = it.first;
        metadataPair.value = it.second;
        metadataList.push_back(metadataPair);
    }

    return rc;
}

Error DmVolumeCatalog::getVolumeObjects(fds_volid_t volId, std::set<ObjectID> & objIds) {
    GET_VOL_N_CHECK_DELETED(volId);
    HANDLE_VOL_NOT_ACTIVATED();

    std::vector<BlobMetaDesc> blobMetaList;
    Error rc = vol->getAllBlobMetaDesc(blobMetaList);
    if (!rc.ok()) {
        LOGERROR << "Failed to retrieve volume metadata for volume: '" << std::hex
                << volId << std::dec << "' error: '" << rc << "'";
        return rc;
    }

    for (const auto & it : blobMetaList) {
        fds_uint64_t lastObjectSize = DmVolumeCatalog::getLastObjSize(it.desc.blob_size,
                vol->getObjSize());
        fds_uint64_t lastObjOffset = it.desc.blob_size ?
                it.desc.blob_size - lastObjectSize : 0;

        BlobObjList objList;
        rc = vol->getObject(it.desc.blob_name, 0, lastObjOffset, objList);
        if (!rc.ok()) {
            LOGERROR << "Failed to retrieve objects for blob: '" << it.desc.blob_name <<
                    "' volume: '" << std::hex << volId << std::dec << "'";
            return rc;
        }

        for (const auto & obj : objList) {
            objIds.insert(obj.second.oid);
        }
    }

    return rc;
}

Error DmVolumeCatalog::getBlobMeta(fds_volid_t volId, const std::string & blobName,
        blob_version_t * blobVersion, fds_uint64_t * blobSize,
        fpi::FDSP_MetaDataList * metaList) {
    LOGDEBUG << "Will retrieve metadata for blob '" << blobName << "' volume: '"
            << std::hex << volId << std::dec << "'";

    BlobMetaDesc blobMeta;
    Error rc = getBlobMetaDesc(volId, blobName, blobMeta);
    if (rc.ok()) {
        // fill in version, size and meta list
        if (blobVersion)
            { *blobVersion = blobMeta.desc.version; }
        if (blobSize)
            { *blobSize = blobMeta.desc.blob_size; }
        if (metaList)
            { blobMeta.meta_list.toFdspPayload(*metaList); }
    }

    return rc;
}

Error DmVolumeCatalog::getBlob(fds_volid_t volId, const std::string& blobName,
                                 fds_uint64_t startOffset, fds_int64_t endOffset,
                                 blob_version_t* blobVersion, fpi::FDSP_MetaDataList* metaList,
                                 fpi::FDSP_BlobObjectList* objList, fds_uint64_t* blobSize) {
    LOGDEBUG << "Will retrieve blob '" << blobName << "' offset '" << startOffset <<
            "' volume '" << std::hex << volId << std::dec << "'";

    *blobSize = 0;
    Error rc = getBlobMeta(volId, blobName, blobVersion, blobSize, metaList);
    if (!rc.ok()) {
        LOGNOTIFY << "Failed to retrieve blob '" << blobName << "' volume '" <<
                std::hex << volId << std::dec << "'";
        return rc;
    }

    if (0 == *blobSize) {
        // empty blob
        return rc;
    } else if (startOffset >= *blobSize) {
        return ERR_BLOB_OFFSET_INVALID;
    } else if (endOffset >= static_cast<fds_int64_t>(*blobSize)) {
        endOffset = -1;
    }

    GET_VOL_N_CHECK_DELETED(volId);

    fds_uint64_t lastObjectSize = DmVolumeCatalog::getLastObjSize(*blobSize, vol->getObjSize());
    if (endOffset < 0) {
        endOffset = *blobSize ? *blobSize - lastObjectSize : 0;
    }

    rc = vol->getObject(blobName, startOffset, endOffset, *objList);

    if (rc.ok()) {
        fpi::FDSP_BlobObjectList::reverse_iterator iter = objList->rbegin();
        if (objList->rend() != iter) {
            const fds_uint64_t lastOffset =
                    DmVolumeCatalog::getLastOffset(*blobSize, vol->getObjSize());
            if (static_cast<fds_int64_t>(lastOffset) == iter->offset) {
                iter->size = lastObjectSize;
            }
        }
    }

    return rc;
}

Error DmVolumeCatalog::getBlobAndMetaFromSnapshot(fds_volid_t volId,
                                                  const std::string& blobName,
                                                  BlobMetaDesc &meta,
                                                  fpi::FDSP_BlobObjectList& obj_list,
                                                  const Catalog::MemSnap snap) {
    GET_VOL_N_CHECK_DELETED(volId);
    HANDLE_VOL_NOT_ACTIVATED();

    Error rc = vol->getBlobMetaDesc(blobName, meta, snap);

    fds_uint64_t reverse_engineer_last_offset = (std::numeric_limits<fds_uint32_t>::max()-1)* vol->getObjSize();

    rc = vol->getObject(blobName, 0, reverse_engineer_last_offset, obj_list, snap);

    return rc;
}

Error DmVolumeCatalog::listBlobs(fds_volid_t volId, fpi::BlobDescriptorListType* bDescrList) {
    GET_VOL_N_CHECK_DELETED(volId);
    HANDLE_VOL_NOT_ACTIVATED();

    std::vector<BlobMetaDesc> blobMetaList;
    Error rc = vol->getAllBlobMetaDesc(blobMetaList);
    if (!rc.ok()) {
        LOGERROR << "Failed to retrieve volume metadata for volume: '" << std::hex
                << volId << std::dec << "' error: '" << rc << "'";
        return rc;
    }

    for (const auto & it : blobMetaList) {
        fpi::BlobDescriptor bdescr;
        bdescr.name = it.desc.blob_name;
        bdescr.byteCount = it.desc.blob_size;
        bdescr.metadata = it.meta_list;
        bDescrList->push_back(bdescr);
    }
    return rc;
}

Error DmVolumeCatalog::listBlobsWithPrefix (fds_volid_t volId,
                                            std::string const& prefix,
                                            std::string const& delimiter,
                                            fpi::BlobDescriptorListType& results,
                                            std::vector<std::string>& skippedPrefixes)
{
    GET_VOL_N_CHECK_DELETED(volId);
    HANDLE_VOL_NOT_ACTIVATED();

    std::vector<BlobMetaDesc> blobMetaList;
    Error rc = vol->getBlobMetaDescForPrefix(prefix, delimiter, blobMetaList, skippedPrefixes);
    if (!rc.ok())
    {
        LOGERROR << "Failed to retrieve volume metadata for volume: '" << std::hex
                 << volId << std::dec << "' error: '" << rc << "'";
        return rc;
    }

    for (auto const& blobMetadata : blobMetaList)
    {
        fpi::BlobDescriptor descriptor;
        descriptor.name = blobMetadata.desc.blob_name;
        descriptor.byteCount = blobMetadata.desc.blob_size;
        descriptor.metadata = blobMetadata.meta_list;

        results.push_back(descriptor);
    }

    return rc;
}

Error DmVolumeCatalog::getObjectIds(fds_volid_t volId,
                                    const uint32_t &maxObjs,
                                    const Catalog::MemSnap &snap,
                                    std::unique_ptr<Catalog::catalog_iterator_t>& dbItr,
                                    std::list<ObjectID> &objects) {
    GET_VOL_N_CHECK_DELETED(volId);
    vol->getObjectIds(maxObjs, snap, dbItr, objects);
    return ERR_OK;
}

Error DmVolumeCatalog::putBlobMeta(fds_volid_t volId, const std::string& blobName,
        const MetaDataList::const_ptr& metaList, const BlobTxId::const_ptr& txId,
        const sequence_id_t seq_id) {
    LOGDEBUG << "Will commit metadata for volume '" << std::hex << volId << std::dec <<
            "' blob '" << blobName << "'";

    BlobMetaDesc blobMeta;
    Error rc = getBlobMetaDesc(volId, blobName, blobMeta);
    if (rc.ok() || rc == ERR_CAT_ENTRY_NOT_FOUND) {
        if (metaList) {
            LOGDEBUG << "Adding metadata " << *metaList;
            mergeMetaList(blobMeta.meta_list, *metaList);
        }

        // TODO(bszmyd): Sat 29 Aug 2015 10:46:52 AM MDT
        // Renable this when the working
        // ENSURE_SEQUENCE_ADV(blobMeta.desc.sequence_id, seq_id, volId, blobName);

        blobMeta.desc.version += 1;
        blobMeta.desc.sequence_id = std::max(blobMeta.desc.sequence_id, seq_id);
        if (ERR_CAT_ENTRY_NOT_FOUND == rc) {
            blobMeta.desc.blob_name = blobName;
        }

        GET_VOL_N_CHECK_DELETED(volId);

        rc = vol->putBlobMetaDesc(blobName, blobMeta);
    }

    if (!rc.ok()) {
        LOGERROR << "Failed to update blob metadata volume: '" << std::hex << volId
                << std::dec << "' blob: '" << blobName << "'";
    }

    return rc;
}

Error DmVolumeCatalog::putBlob(fds_volid_t volId, const std::string& blobName,
        const MetaDataList::const_ptr& metaList, const BlobObjList::const_ptr& blobObjList,
        const BlobTxId::const_ptr& txId, const sequence_id_t seq_id ) {
    LOGDEBUG << "Will commit blob: '" << blobName << "' to volume: '" << std::hex << volId
            << std::dec << "'; " << *blobObjList;
    // do not use this method if blob_obj_list is empty
    fds_assert(0 < blobObjList->size());

    bool newBlob = false;

    // see if the blob exists and get details of it
    BlobMetaDesc blobMeta;
    Error rc = getBlobMetaDesc(volId, blobName, blobMeta);
    if (!rc.ok()) {
        if (rc == ERR_CAT_ENTRY_NOT_FOUND) {
            newBlob = true;
        } else {
            LOGERROR << "Failed to retrieve blob details for volume: '" << std::hex << volId
                    << std::dec << "' blob: '" << blobName << "'";
            return rc;
        }
    }

    GET_VOL_N_CHECK_DELETED(volId);
    // TODO(bszmyd): Sat 29 Aug 2015 10:46:52 AM MDT
    // Renable this when the working
    // ENSURE_SEQUENCE_ADV(blobMeta.desc.sequence_id, seq_id, volId, blobName);

    // verify object size assumption holds
    rc = blobObjList->verify(vol->getObjSize());
    if (!rc.ok()) {
        LOGERROR << "Failed to verify updated object list for blob update: " << rc;
        return rc;
    }

    std::vector<ObjectID> expungeList;

    BlobObjList::const_iter firstIter = blobObjList->begin();
    fds_verify(blobObjList->end() != firstIter);
    fds_uint64_t reqFirstOffset = firstIter->first;

    // existing blob details
    const fds_uint64_t oldBlobSize = blobMeta.desc.blob_size;
    const fds_uint32_t oldLastObjSize = DmVolumeCatalog::getLastObjSize(oldBlobSize,
            vol->getObjSize());
    // oldLastOffset -> the offset that the old blob ends on prior to the new put
    const fds_uint64_t oldLastOffset = oldBlobSize ? oldBlobSize - oldLastObjSize : 0;
    BlobObjList oldBlobObjList;

    // new details
    fds_uint64_t newBlobSize = oldBlobSize;
    fds_uint64_t newLastOffset = blobObjList->lastOffset();

    if (!newBlob) {
        rc = vol->getObject(blobName, reqFirstOffset, newLastOffset, oldBlobObjList);
        if (!rc.ok()) {
            LOGERROR << "Failed to retrieve existing blob: '" << blobName << "' in volume: '"
                    << std::hex << volId << std::dec <<"'";
            return rc;
        }

        if (reqFirstOffset <= oldLastOffset && newLastOffset >= oldLastOffset) {
            oldBlobObjList[oldLastOffset].size = oldLastObjSize;
        }
    }

    for (BlobObjList::const_iter cit = blobObjList->begin(); blobObjList->end() != cit; ++cit) {
        BlobObjList::iterator oldIter = oldBlobObjList.find(cit->first);
        if (oldBlobObjList.end() == oldIter) {
            // new offset, update blob size
            newBlobSize = std::max(newBlobSize, cit->first + cit->second.size);

            FDSGUARD(lockVolSummaryMap_);
            DmVolumeSummaryMap_t::iterator volSummaryIter = volSummaryMap_.find(volId);
            if (volSummaryMap_.end() != volSummaryIter) {
                volSummaryIter->second->objectCount.fetch_add(1);
            }
            continue;
        }

        if (NullObjectID != oldIter->second.oid) {
            // null object does not physically exist
            expungeList.push_back(oldIter->second.oid);
        }

        // if we are updating last offset, adjust blob size
        if (oldLastOffset == cit->first) {
            // modifying existing last offset, update size
            if (NullObjectID != oldIter->second.oid) {
                fds_verify(newBlobSize >= oldLastObjSize);
                newBlobSize -= oldLastObjSize;
            }
            newBlobSize += cit->second.size;
        } else if (cit->first == newLastOffset) {
            // fds_verify(oldIter->second.oid != NullObjectID);
            fds_verify(newBlobSize >= vol->getObjSize());
            newBlobSize -= vol->getObjSize();
            newBlobSize += cit->second.size;
        }
        // otherwise object sizes match
    }

    std::vector<fds_uint64_t> delOffsetList;
    if (blobObjList->endOfBlob() && newLastOffset < oldLastOffset) {
        // truncating the blob
        BlobObjList truncateObjList;
        rc = vol->getObject(blobName, newLastOffset + vol->getObjSize(),
                oldLastOffset, truncateObjList);
        if (!rc.ok()) {
            LOGERROR << "Failed to retrieve existing blob: '" << blobName << "' in volume: '"
                    << std::hex << volId << std::dec <<"' for truncation";
            return rc;
        }

        for (auto & i : truncateObjList) {
            if (NullObjectID != i.second.oid) {
                delOffsetList.push_back(i.first);
                expungeList.push_back(i.second.oid);
                newBlobSize -= i.first == oldLastOffset ? oldLastObjSize : i.second.size;
            }
        }

        FDSGUARD(lockVolSummaryMap_);
        DmVolumeSummaryMap_t::iterator volSummaryIter = volSummaryMap_.find(volId);
        if (volSummaryMap_.end() != volSummaryIter) {
            volSummaryIter->second->objectCount.fetch_sub(truncateObjList.size());
        }
    }

    // Is it possible to have an empty incoming metaList but with one or more offsets?
    // We certainly have hit a case in testing, and updateFwdCommittedBlob seems to think
    // it's possible.
    if (metaList != nullptr) {
        mergeMetaList(blobMeta.meta_list, *metaList);
    }
    blobMeta.desc.version += 1;
    blobMeta.desc.sequence_id = std::max(blobMeta.desc.sequence_id, seq_id);
    blobMeta.desc.blob_size = newBlobSize;
    if (newBlob) {
        blobMeta.desc.blob_name = blobName;
    }

    rc = vol->putBatch(blobName, blobMeta, *blobObjList, delOffsetList);
    if (!rc.ok()) {
        LOGERROR << "Failed to put blob: '" << blobName << "' in volume: '" << std::hex
                << volId << std::dec << "' error: '" << rc << "'";
        return rc;
    }

    synchronized(lockVolSummaryMap_) {
        DmVolumeSummaryMap_t::iterator volSummaryIter = volSummaryMap_.find(volId);
        if (volSummaryMap_.end() != volSummaryIter) {
            fds_uint64_t diff = 0;
            if (newBlobSize >= oldBlobSize) {
                diff = newBlobSize - oldBlobSize;
                volSummaryIter->second->size.fetch_add(diff);
            } else {
                diff = oldBlobSize - newBlobSize;
                volSummaryIter->second->size.fetch_sub(diff);
            }
            if (newBlob) {
                volSummaryIter->second->blobCount.fetch_add(1);
            }
        }
    }

    // actually expunge objects that were dereferenced by the blob
    // TODO(xxx): later that should become part of GC and done in background
    fds_verify(expungeCb_);
    return expungeCb_(volId, expungeList, false);
}

// NOTE: used by the Batch ifdef, not currently called by compiled code
Error DmVolumeCatalog::putBlob(fds_volid_t volId, const std::string& blobName,
        fds_uint64_t blobSize, const MetaDataList::const_ptr& metaList,
        CatWriteBatch & wb, const sequence_id_t seq_id, bool truncate /* = true */) {
    LOGDEBUG << "Will commit blob: '" << blobName << "' to volume: '" << std::hex << volId
            << std::dec << "'";

    bool newBlob = false;
    // see if the blob exists and get details of it
    BlobMetaDesc blobMeta;
    Error rc = getBlobMetaDesc(volId, blobName, blobMeta);
    if (!rc.ok()) {
        if (rc == ERR_CAT_ENTRY_NOT_FOUND) {
            newBlob = true;
        } else {
            LOGERROR << "Failed to retrieve blob details for volume: '" << std::hex << volId
                    << std::dec << "' blob: '" << blobName << "'";
            return rc;
        }
    }

    GET_VOL_N_CHECK_DELETED(volId);

    fds_uint64_t oldBlobSize = blobMeta.desc.blob_size;
    fds_uint32_t oldObjCount = oldBlobSize / vol->getObjSize();
    if (0 != oldBlobSize && oldBlobSize % vol->getObjSize()) {
        oldObjCount += 1;
    }

    fds_uint64_t newBlobSize = (oldBlobSize < blobSize || truncate) ? blobSize : oldBlobSize;
    fds_uint64_t newObjCount = newBlobSize / vol->getObjSize();
    if (0 != newBlobSize && newBlobSize % vol->getObjSize()) {
        newObjCount += 1;
    }

    // Is it possible to have an empty incoming metaList but with one or more offsets?
    // We certainly have hit a case in testing, and updateFwdCommittedBlob seems to think
    // it's possible.
    if (metaList != nullptr) {
        mergeMetaList(blobMeta.meta_list, *metaList);
    }
    blobMeta.desc.version += 1;
    blobMeta.desc.sequence_id = seq_id;
    blobMeta.desc.blob_size = newBlobSize;
    if (newBlob) {
        blobMeta.desc.blob_name = blobName;
    }

    rc = vol->putBatch(blobName, blobMeta, wb);
    if (!rc.ok()) {
        LOGERROR << "Failed to put blob: '" << blobName << "' in volume: '" << std::hex
                << volId << std::dec << "' error: '" << rc << "'";
        return rc;
    }

    FDSGUARD(lockVolSummaryMap_);
    DmVolumeSummaryMap_t::iterator volSummaryIter = volSummaryMap_.find(volId);
    if (volSummaryMap_.end() != volSummaryIter) {
        fds_uint64_t diff = 0;
        if (newBlobSize >= oldBlobSize) {
            diff = newBlobSize - oldBlobSize;
            volSummaryIter->second->size.fetch_add(diff);
        } else {
            diff = oldBlobSize - newBlobSize;
            volSummaryIter->second->size.fetch_sub(diff);
        }

        diff = 0;
        if (newObjCount >= oldObjCount) {
            diff = newObjCount - oldObjCount;
            volSummaryIter->second->objectCount.fetch_add(diff);
        } else {
            diff = oldObjCount - newObjCount;
            volSummaryIter->second->objectCount.fetch_sub(diff);
        }

        if (newBlob) {
            volSummaryIter->second->blobCount.fetch_add(1);
        }
    }

    return rc;
}

Error DmVolumeCatalog::flushBlob(fds_volid_t volId, const std::string& blobName) {
    // TODO(umesh): not sure if this is needed
    return ERR_OK;
}

Error DmVolumeCatalog::removeVolumeMeta(fds_volid_t volId) {
    // TODO(umesh): implement this
    return ERR_OK;
}

Error DmVolumeCatalog::deleteBlob(fds_volid_t volId, const std::string& blobName,
            blob_version_t blobVersion, bool const expunge_data) {
    LOGDEBUG << "Will delete blob: '" << blobName << "' volume: '" << std::hex << volId
            << std::dec << "' expunge: " << expunge_data;

    BlobMetaDesc blobMeta;
    Error rc = getBlobMetaDesc(volId, blobName, blobMeta);
    if (!rc.ok()) {
        LOGNOTIFY << "Blob '" << blobName << "' not found!";
        return rc;
    }

    GET_VOL_N_CHECK_DELETED(volId);

    fds_uint64_t lastObjectSize = DmVolumeCatalog::getLastObjSize(blobMeta.desc.blob_size,
                vol->getObjSize());
    fds_uint64_t endOffset = blobMeta.desc.blob_size ?
            blobMeta.desc.blob_size - lastObjectSize : 0;

    fpi::FDSP_BlobObjectList objList;
    rc = vol->getObject(blobName, 0, endOffset, objList);
    if (!rc.ok()) {
        LOGWARN << "Failed to retrieve objects for blob: '" << blobName << "'";
        return rc;
    }

    std::vector<ObjectID> expungeList;
    for (const auto & it : objList) {
        const ObjectID obj(it.data_obj_id.digest);
        if (NullObjectID != obj) {
            expungeList.push_back(obj);
        }
    }
    bool fIsSnapshot = vol->isSnapshot();
    rc = vol->deleteObject(blobName, 0, endOffset);
    if (rc.ok()) {
        rc = vol->deleteBlobMetaDesc(blobName);
        if (!rc.ok()) {
            LOGWARN << "Failed to delete metadata for blob: '" << blobName << "'";
        }

        synchronized(lockVolSummaryMap_) {
            DmVolumeSummaryMap_t::iterator iter = volSummaryMap_.find(volId);
            if (volSummaryMap_.end() != iter) {
                iter->second->blobCount.fetch_sub(1);
                iter->second->size.fetch_sub(blobMeta.desc.blob_size);
                iter->second->objectCount.fetch_sub(expungeList.size());
            }
        }

        // actually expunge objects that were dereferenced by the blob
        // TODO(xxx): later that should become part of GC and done in background
        fds_assert(expungeCb_);
        if (expunge_data) {
            return expungeCb_(volId, expungeList, fIsSnapshot);
        }
    }

    return rc;
}

Error DmVolumeCatalog::syncCatalog(fds_volid_t volId, const NodeUuid& dmUuid) {
    GET_VOL_N_CHECK_DELETED(volId);
    return vol->syncCatalog(dmUuid);
}

Error DmVolumeCatalog::migrateDescriptor(fds_volid_t volId,
                                         const std::string& blobName,
                                         const std::string& blobData){
    GET_VOL_N_CHECK_DELETED(volId);
    Error err;
    bool fTruncate = true;

    /* This is actually the volume descriptor, but we did fancy hacks
       (empty blob name) to avoid a special case message for it */
    if (blobName.size() == 0) {
        fpi::FDSP_MetaDataList metadataList;
        VolumeMetaDesc newDesc(metadataList, 0);
        err = newDesc.loadSerialized(blobData);

        if (err.ok()) {
            err = vol->putVolumeMetaDesc(newDesc);

            if (!err.ok()) {
                LOGERROR << "Failed to insert migrated Volume Descriptor into catalog for volume: "
                         << volId << " error: " << err;
            }
        }else{
            LOGERROR << "Failed to deserialize migrated Volume Descriptor for volume: "
                     << volId << " error: " << err;
        }

        return err;
    }

    // This is actually a delete (empty blob data)
    if (blobData.size() == 0) {
        // version is ignored, so set to zero
        err = deleteBlob(volId, blobName, 0);

        if (!err.ok()) {
            LOGERROR << "During migration, failed to delete blob: " << blobName
                     << " from catalog for volume: " << volId << " error: "
                     << err;
        }

        return err;
    }

    // This is really a blob descriptor update. we may need to trunctate the offsets.
    BlobMetaDesc oldBlob;
    err = vol->getBlobMetaDesc(blobName, oldBlob);

    if (ERR_CAT_ENTRY_NOT_FOUND == err) {
        fTruncate = false;
    }else if (!err.ok()) {
        LOGERROR << "During migration, failed to read existing blob: " << blobName
                 << " from catalog for volume: " << volId << " error: "
                 << err;
        return err;
    }

    BlobMetaDesc newBlob;
    err = newBlob.loadSerialized(blobData);

    if (!err.ok()) {
        LOGERROR << "Failed to deserialize migrated blob: " << blobName
                 << " from catalog for volume: " << volId << " error: " << err;
        return err;
    }

    if (oldBlob.desc.sequence_id >= newBlob.desc.sequence_id) {
        LOGMIGRATE << "Overwriting blob with older sequence id version vol:"
                   << volId << " blob: " << blobName << " old seq_id: "
                   << oldBlob.desc.sequence_id << " new seq_id: "
                   << newBlob.desc.sequence_id;
    }

    if (fTruncate) {
        const fds_uint64_t oldLastOffset = DmVolumeCatalog::getLastOffset(oldBlob.desc.blob_size,
                                                                          vol->getObjSize());

        const fds_uint64_t newLastOffset = DmVolumeCatalog::getLastOffset(newBlob.desc.blob_size,
                                                                          vol->getObjSize());

        if ((newLastOffset+1) < oldLastOffset) {
            // delete starting at the ofset after the new last offset
            LOGDEBUG << "deleteObject start " << blobName << " newLastOffset: " << newLastOffset << " oldLastOffset: " << oldLastOffset;

            err = vol->deleteObject(blobName, newLastOffset +1, oldLastOffset);
            LOGDEBUG << "deleteObject end " << blobName << " newLastOffset: " << newLastOffset << " oldLastOffset: " << oldLastOffset;

            if (!err.ok()) {
                LOGERROR << "During migration, failed to truncate blob: "
                         << blobName << " from catalog for volume: " << volId
                         << " error: " << err;
                return err;
            }
        }
    }

    err = vol->putBlobMetaDesc(blobName, newBlob);

    if (!err.ok()) {
        LOGERROR << "Failed to insert migrated blob: " << blobName
                 << " into catalog for volume: " << volId << " error: " << err;
    }

    return err;
}

DmPersistVolCat::ptr DmVolumeCatalog::getVolume(fds_volid_t volId) {
    GET_VOL(volId);
    return vol;
}

Error DmVolumeCatalog::getVolumeSequenceId(fds_volid_t volId, sequence_id_t& seq_id) {
    GET_VOL_N_CHECK_DELETED(volId);
    return vol->getLatestSequenceId(seq_id);
}

Error DmVolumeCatalog::getAllBlobsWithSequenceId(fds_volid_t volId, std::map<std::string, int64_t>& blobsSeqId,
                                                 Catalog::MemSnap snap) {
    GET_VOL_N_CHECK_DELETED(volId);
    return vol->getAllBlobsWithSequenceId(blobsSeqId, snap);
}

Error DmVolumeCatalog::putObject(fds_volid_t volId,
                                 const std::string & blobName,
                                 const BlobObjList & objs)
{
    GET_VOL_N_CHECK_DELETED(volId);
    return vol->putObject(blobName, objs);
}

Error DmVolumeCatalog::getVolumeSnapshot(fds_volid_t volId, Catalog::MemSnap &snap) {
    GET_VOL_N_CHECK_DELETED(volId);
    return vol->getInMemorySnapshot(snap);
}

Error DmVolumeCatalog::freeVolumeSnapshot(fds_volid_t volId, Catalog::MemSnap &snap) {
    GET_VOL(volId);
    return vol->freeInMemorySnapshot(snap);
}

Error DmVolumeCatalog::forEachObject(fds_volid_t volId, std::function<void(const ObjectID&)> func) {
    GET_VOL_N_CHECK_DELETED(volId);
    vol->forEachObject(func);
    return ERR_OK;
}

}  // namespace fds
