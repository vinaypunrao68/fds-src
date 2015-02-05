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
        } \

namespace fds {

DmVolumeCatalog::DmVolumeCatalog(char const * const name) : Module(name), expungeCb_(0) {}

DmVolumeCatalog::~DmVolumeCatalog() {}

Error DmVolumeCatalog::getBlobMetaDesc(fds_volid_t volId, const std::string & blobName,
            BlobMetaDesc & blobMeta) {
    GET_VOL_N_CHECK_DELETED(volId);

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
                    voldesc.isSnapshot(), voldesc.isSnapshot(),
                    voldesc.isSnapshot() ? voldesc.srcVolumeId : invalid_vol_id));
    /*
    } else {
        vol.reset(new DmPersistVolFile(voldesc.volUUID, voldesc.maxObjSizeInBytes,
                    voldesc.isSnapshot(), voldesc.isSnapshot(),
                    voldesc.isSnapshot() ? voldesc.srcVolumeId : invalid_vol_id));
    }
    */

    FDSGUARD(volMapLock_);
    fds_verify(volMap_.end() == volMap_.find(voldesc.volUUID));
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
    oss << root->dir_user_repo_dm() << voldesc.srcVolumeId << "/" << voldesc.srcVolumeId
            << "_vcat.ldb";
    std::string dbDir = oss.str();

    oss.clear();
    oss.str("");
    oss << root->dir_user_repo_dm();
    if (!voldesc.isSnapshot()) {
        oss <<  voldesc.volUUID;
    } else {
        oss << voldesc.srcVolumeId << "/snapshot";
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
                    voldesc.isSnapshot(), voldesc.srcVolumeId));
        /*
        } else {
            vol.reset(new DmPersistVolFile(voldesc.volUUID, objSize, voldesc.isSnapshot(),
                    voldesc.isSnapshot(), voldesc.srcVolumeId));
        }
        */

        FDSGUARD(volMapLock_);
        fds_verify(volMap_.end() == volMap_.find(voldesc.volUUID));
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

Error DmVolumeCatalog::deleteEmptyCatalog(fds_volid_t volId) {
    LOGDEBUG << "Will delete catalog for volume '" << std::hex << volId << std::dec << "'";

    synchronized(volMapLock_) {
        std::unordered_map<fds_volid_t, DmPersistVolCat::ptr>::iterator iter =
                volMap_.find(volId);
        if (volMap_.end() != iter && iter->second->isMarkedDeleted()) {
            volMap_.erase(iter);
        }
    }

    return ERR_OK;
}

Error DmVolumeCatalog::getVolumeMeta(fds_volid_t volId, fds_uint64_t* volSize,
        fds_uint64_t* blobCount, fds_uint64_t* objCount) {
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

    rc = getVolumeMetaInternal(volId, volSize, blobCount, objCount);

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

Error DmVolumeCatalog::getVolumeMetaInternal(fds_volid_t volId, fds_uint64_t * volSize,
            fds_uint64_t * blobCount, fds_uint64_t * objCount) {
    GET_VOL_N_CHECK_DELETED(volId);

    std::vector<BlobMetaDesc> blobMetaList;
    Error rc = vol->getAllBlobMetaDesc(blobMetaList);
    if (!rc.ok()) {
        LOGERROR << "Failed to retrieve volume metadata for volume: '" << std::hex
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

Error DmVolumeCatalog::getVolumeObjects(fds_volid_t volId, std::set<ObjectID> & objIds) {
    GET_VOL_N_CHECK_DELETED(volId);

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
        *blobVersion = blobMeta.desc.version;
        *blobSize = blobMeta.desc.blob_size;
        blobMeta.meta_list.toFdspPayload(*metaList);
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
        return ERR_CAT_ENTRY_NOT_FOUND;
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

Error DmVolumeCatalog::listBlobs(fds_volid_t volId, fpi::BlobInfoListType* binfoList) {
    GET_VOL_N_CHECK_DELETED(volId);

    std::vector<BlobMetaDesc> blobMetaList;
    Error rc = vol->getAllBlobMetaDesc(blobMetaList);
    if (!rc.ok()) {
        LOGERROR << "Failed to retrieve volume metadata for volume: '" << std::hex
                << volId << std::dec << "' error: '" << rc << "'";
        return rc;
    }

    for (const auto & it : blobMetaList) {
        fpi::FDSP_BlobInfoType binfo;
        binfo.blob_name = it.desc.blob_name;
        binfo.blob_size = it.desc.blob_size;
        binfo.metadata = it.meta_list;
        binfoList->push_back(binfo);
    }
    return rc;
}

Error DmVolumeCatalog::putBlobMeta(fds_volid_t volId, const std::string& blobName,
        const MetaDataList::const_ptr& metaList, const BlobTxId::const_ptr& txId) {
    LOGDEBUG << "Will commit metadata for volume '" << std::hex << volId << std::dec <<
            "' blob '" << blobName << "'";

    BlobMetaDesc blobMeta;
    Error rc = getBlobMetaDesc(volId, blobName, blobMeta);
    if (rc.ok() || rc == ERR_CAT_ENTRY_NOT_FOUND) {
        if (metaList) {
            LOGDEBUG << "Adding metadata " << *metaList;
            mergeMetaList(blobMeta.meta_list, *metaList);
        }
        blobMeta.desc.version += 1;
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
        const BlobTxId::const_ptr& txId) {
    LOGDEBUG << "Will commit blob: '" << blobName << "' to volume: '" << std::hex << volId
            << std::dec << "'; " << *blobObjList;
    // do not use this method if blob_obj_list is empty
    fds_verify(0 < blobObjList->size());

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

    // verify object size assumption holds
    blobObjList->verify(vol->getObjSize());

    std::vector<ObjectID> expungeList;

    BlobObjList::const_iter firstIter = blobObjList->begin();
    fds_verify(blobObjList->end() != firstIter);
    fds_uint64_t reqFirstOffset = firstIter->first;

    // existing blob details
    const fds_uint64_t oldBlobSize = blobMeta.desc.blob_size;
    const fds_uint32_t oldLastObjSize = DmVolumeCatalog::getLastObjSize(oldBlobSize,
            vol->getObjSize());
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
            fds_verify(oldIter->second.oid != NullObjectID);
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

    mergeMetaList(blobMeta.meta_list, *metaList);
    blobMeta.desc.version += 1;
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
    return expungeCb_(volId, expungeList);
}

Error DmVolumeCatalog::putBlob(fds_volid_t volId, const std::string& blobName,
        fds_uint64_t blobSize, const MetaDataList::const_ptr& metaList,
        CatWriteBatch & wb, bool truncate /* = true */) {
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

    mergeMetaList(blobMeta.meta_list, *metaList);
    blobMeta.desc.version += 1;
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
            blob_version_t blobVersion) {
    LOGDEBUG << "Will delete blob: '" << blobName << "' volume: '" << std::hex << volId
            << std::dec << "'";

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
        fds_verify(expungeCb_);
        return expungeCb_(volId, expungeList);
    }

    return rc;
}

Error DmVolumeCatalog::syncCatalog(fds_volid_t volId, const NodeUuid& dmUuid) {
    GET_VOL_N_CHECK_DELETED(volId);
    return vol->syncCatalog(dmUuid);
}

DmPersistVolCat::ptr DmVolumeCatalog::getVolume(fds_volid_t volId) {
    GET_VOL(volId);
    return vol;
}
}  // namespace fds
