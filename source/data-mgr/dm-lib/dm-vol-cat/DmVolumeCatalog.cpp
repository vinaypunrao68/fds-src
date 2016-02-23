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

#include <VolumeMeta.h>

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

DmVolumeCatalog::DmVolumeCatalog(CommonModuleProviderIf* modProvider,
                                 char const * const name)
    : HasModuleProvider(modProvider),
    Module(name), expungeCb_(0) {}

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
    fds_bool_t fArchiveLogs = CONFIG_BOOL("fds.feature_toggle.common.enable_timeline", true) &&
            !voldesc.isSnapshot() && voldesc.contCommitlogRetention > 0;
        vol.reset(new DmPersistVolDB(MODULEPROVIDER(),
                    voldesc.volUUID, voldesc.maxObjSizeInBytes,
                                     voldesc.isSnapshot(), voldesc.isSnapshot(), voldesc.isClone(),
                                     fArchiveLogs,
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

    const FdsRootDir* root = MODULEPROVIDER()->proc_fdsroot();
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
        fds_bool_t fArchiveLogs = CONFIG_BOOL("fds.feature_toggle.common.enable_timeline", true) &&
                !voldesc.isSnapshot() && voldesc.contCommitlogRetention > 0;

            vol.reset(new DmPersistVolDB(MODULEPROVIDER(),
                                         voldesc.volUUID, objSize, voldesc.isSnapshot(),
                                         voldesc.isSnapshot(), voldesc.isClone(), fArchiveLogs, voldesc.srcVolumeId));
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
    LOGNORMAL << "Will reload catalog for volume " << voldesc.volUUID;
    deleteCatalog(voldesc.volUUID, false);
    Error rc = addCatalog(voldesc);
    if (!rc.ok()) {
        LOGWARN << "Failed to re-instantiate the volume '" << voldesc.volUUID;
        return rc;
    }
    rc = activateCatalog(voldesc.volUUID);

    if (rc.ok()) {
        /*
        GET_VOL(volId);
        vol->resetVolSummary();
        fds_uint64_t volSize=0, blobCount=0, objCount=0;
        // statVolumeLogical(voldesc.volUUID, &volSize, &blobCount, &objCount);
        LOGNORMAL << "reloaded vol:" << voldesc.volUUID << "["
                  << " size:" << volSize
                  << " blobs:" << blobCount
                  << " objects:" << objCount << "]";
        */
    } else {
        LOGWARN << "unable to activate vol:" << voldesc.volUUID
                << "error:" << rc;
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

Error DmVolumeCatalog::deleteCatalog(fds_volid_t volId, bool checkDeleted /* = true */) {
    LOGDEBUG << "will delete catalog for vol:" << volId;
    GET_VOL(volId);
    if (!vol) return ERR_VOL_NOT_FOUND;

    // wait for all mem snapshots to be released
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

Error DmVolumeCatalog::statVolumeLogical(fds_volid_t volId,
                                         fds_uint64_t* volSize,
                                         fds_uint64_t* blobCount,
                                         fds_uint64_t* objCount) {
    Error rc(ERR_OK);

    GET_VOL_N_CHECK_DELETED(volId);

    if (!vol->volSummaryInitialized()) {
        /**
         * We need to initialize the volume summary cache.
         */
        rc = statVolumeInternal(volId, volSize, blobCount, objCount);

        if (rc.ok()) {
            if (vol->initVolSummary(*volSize, *blobCount, *objCount) == ERR_DUPLICATE) {
                /**
                 * Someone got to the initialization ahead of us. Just get
                 * the current summary as if we never suspected initialization
                 * had not been done.
                 */
                rc = vol->getVolSummary(volSize, blobCount, objCount);
            }
        }
    } else {
        rc = vol->getVolSummary(volSize, blobCount, objCount);
    }

    return rc;
}

Error DmVolumeCatalog::statVolumePhysical(fds_volid_t volId, fds_uint64_t *pbytes, fds_uint64_t *pObjCount) {
    GET_VOL_N_CHECK_DELETED(volId);
    HANDLE_VOL_NOT_ACTIVATED();

    std::vector<BlobMetaDesc> blobMetaList;

    *pbytes = 0;
    *pObjCount = 0;

    Error rc = vol->getAllBlobMetaDesc(blobMetaList);
    if (!rc.ok()) {
        LOGERROR << "Failed to retrieve volume metadata for volume: '" << std::hex
                 << volId << std::dec << "' error: '" << rc << "'";
        return rc;
    }

    std::unordered_set<ObjectID, ObjectHash> uniqueObjIds;
    for (const auto & it : blobMetaList) {
        fds_uint64_t endOffset = DmVolumeCatalog::getLastOffset(it.desc.blob_size,
                                                                vol->getObjSize());
        BlobObjList objList;
        rc = vol->getObject(it.desc.blob_name, 0 /* start offset */, endOffset, objList);

        /**
         * The vol->getObject() method (which should probably really be called something like
         * getDataObjectRange()), makes an assumption that all Data Objects in the
         * range are written and all have the same size, the maximum size of a Data Object
         * as defined with the Volume, here identified with a call to vol->getObjSize().
         *
         * This assumption is wrong on both accounts.
         *
         * The last Data Object in the Blob's list will be sized sufficiently to finish
         * out the entire size of the Blob. In other words, the rest of DM assumes that the
         * Connector wrote the Blob in chunks of vol->getObjSize() size until the last
         * Data Object which catches the remainder of BlobSize/vol->getObjSize(). In order
         * to get a correct PBYTE count here, we must now correct that bad assumption
         * by vol->getObject().
         *
         * We could put this correction into vol->getObject() so that all callers can enjoy
         * the benefits of a more correct Data Object list, but that would break the S3
         * Connector's multi-part upload interface which relies upon the incorrect list
         * produced by vol->getObject().
         *
         * As to the other bad assumption, that all Data Objects in the range have been
         * written, we correct by adding the sizes of only those Data Objects that we
         * actually have - the unique Data Objects, that is.
         */
        objList[endOffset].size = DmVolumeCatalog::getLastObjSize(it.desc.blob_size,
                                                                  vol->getObjSize());

        if (!rc.ok()) {
            LOGERROR << "Failed to retrieve objects for blob: '" << it.desc.blob_name <<
                     "' volume: '" << std::hex << volId << std::dec << "'";
            return rc;
        }

        for (const auto & obj : objList) {
            if (uniqueObjIds.find(obj.second.oid) == uniqueObjIds.end()) {
                uniqueObjIds.insert(obj.second.oid);
                *pbytes += obj.second.size;
            }
        }
    }

    *pObjCount = uniqueObjIds.size();

    return rc;
}

Error DmVolumeCatalog::statVolumeInternal(fds_volid_t volId, fds_uint64_t * volSize,
            fds_uint64_t * blobCount, fds_uint64_t * objCount) {
    GET_VOL_N_CHECK_DELETED(volId);
    HANDLE_VOL_NOT_ACTIVATED();

    *volSize = 0;
    *blobCount = 0;
    *objCount = 0;

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
        /**
         * Get the data object list for this blob.
         *
         * Since we don't know whether and where there might be "sparseness", we'll
         * scan for all possible "offsets" given the (incorrect) logical Blob size.
         *
         * (I put "offsets" in quotes here because
         * we should really be talking about Data Objects. This notion that
         * some how "offsets" equate to Data Objects is an example of the pecularities
         * of a specific storage API - Block, for example - acting as a corrupting
         * influence upon the design and implementation of our storage platform
         * where a clear-eyed assessment of the architecture reveals that no such
         * concept exists. And we would be better off without it in our platform
         * not only because it is an inconsistency with the architecture in this respect,
         * but also because without it we liberate Connectors and applications wishing
         * to write directly to XDI to take full advantage of the freedom of storage
         * management offered by the architecture. With it, everybody has to conform
         * to Block concepts.)
         *
         * The difficulty here is that DM does not correctly implement the
         * architecture where there is no such concept as "offset" and so, no
         * "sparseness".
         */
        BlobObjList objList;
        fds_uint64_t endOffset = DmVolumeCatalog::getLastOffset(it.desc.blob_size,
                                                                vol->getObjSize());
        rc = vol->getObject(it.desc.blob_name, 0 /* start offset */, endOffset, objList);

        if (!rc.ok()) {
            LOGERROR << "Failed to retrieve objects for blob: '" << it.desc.blob_name <<
                     "' volume: '" << volId << "'";
            return rc;
        }

        /**
         * The vol->getObject() method (which should probably really be called something like
         * getDataObjectRange()), makes an assumption that all Data Objects in the
         * range are written and all have the same size, the maximum size of a Data Object
         * as defined with the Volume, here identified with a call to vol->getObjSize().
         *
         * This assumption is wrong on both accounts.
         *
         * The last Data Object in the Blob's list will be sized sufficiently to finish
         * out the entire size of the Blob. In other words, the rest of DM assumes that the
         * Connector wrote the Blob in chunks of vol->getObjSize() size until the last
         * Data Object which catches the remainder of BlobSize/vol->getObjSize(). In order
         * to get a correct byte count here, we must now correct that bad assumption.
         *
         * We could put this correction into vol->getObject() so that all callers can enjoy
         * the benefits of a more correct Data Object list, but that would break the S3
         * Connector's multi-part upload interface which relies upon the incorrect list
         * produced by vol->getObject().
         *
         * As to the other bad assumption, that all Data Objects in the range have been
         * written, we correct and account for that here since we now have the entire
         * Data Object list with no "sparseness".
         */
        objList[objList.lastOffset()].size = DmVolumeCatalog::getLastObjSize(it.desc.blob_size,
                                                                             vol->getObjSize());

        *objCount += objList.size();

        /**
         * WARNING! WARNING! Will Robinson! DANGER!
         *
         * Once we correctly implement the architecture wherein Data Objects may be any
         * size the Connector or XDI application chooses and not restrictied to the
         * arbitrary caprices of whoever set the max Data Object size for the volume ...
         * this won't work!
         */
        *volSize += ((objList.size() - 1) * vol->getObjSize()) + objList[objList.lastOffset()].size;
    }

    LOGDEBUG << "Volume: '" << volId << "' logical size: '" << *volSize
             << "' blobs: '" << *blobCount << "' logical objects: '" << *objCount << "'";

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
    LOGDEBUG << "Will retrieve blob <" << blobName
             << "> start offset <" << startOffset
             << "> end offset <" << endOffset
             << "> volume '" << std::hex << volId << std::dec
             << "'";

    *blobSize = 0;
    Error rc = getBlobMeta(volId, blobName, blobVersion, blobSize, metaList);
    if (!rc.ok()) {
        LOGDEBUG << "Failed to retrieve blob '" << blobName << "' volume '" <<
                std::hex << volId << std::dec << "'";
        return rc;
    }

    if (0 == *blobSize) {
        // empty blob
        return rc;
    } else if (startOffset >= *blobSize) {
        LOGDEBUG  << "For Blob '" << blobName
                  << "', Volume '" << std::hex << volId << std::dec
                  << "', start offset of <" << startOffset
                  << "> not less than Blob size <" << *blobSize
                  << ">.";
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
    fds_uint64_t objectsAdded = 0;  // Number of Data Objects added to the Blob by this operation.
    fds_uint64_t objectsRemoved = 0;  // Number of Data Objects removed from the Blob by this operation.
    fds_uint64_t bytesAdded = 0;  // Number of bytes added to the Blob by this operation.
    fds_uint64_t bytesRemoved = 0;  // Number of bytes removed from the Blob by this operation.

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

    // std::vector<ObjectID> expungeList;

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

            objectsAdded++;
            bytesAdded += cit->second.size; // Data Object size is correct here ... even for the last one. Surprise!
            continue;
        }

        if (NullObjectID != oldIter->second.oid) {
            // null object does not physically exist
            // expungeList.push_back(oldIter->second.oid);
        }

        // if we are updating last offset, adjust blob size
        if (oldLastOffset == cit->first) {
            // modifying existing last offset, update size
            if (NullObjectID != oldIter->second.oid) {
                fds_verify(newBlobSize >= oldLastObjSize);
                newBlobSize -= oldLastObjSize;
            }
            newBlobSize += cit->second.size;

            if (oldLastObjSize >= cit->second.size) {
                bytesRemoved += oldLastObjSize - cit->second.size;
            } else {
                bytesAdded += cit->second.size - oldLastObjSize;
            }
        } else if (cit->first == newLastOffset) {
            // fds_verify(oldIter->second.oid != NullObjectID);
            fds_verify(newBlobSize >= vol->getObjSize());
            newBlobSize -= vol->getObjSize();
            newBlobSize += cit->second.size;

            if (oldLastObjSize >= cit->second.size) {
                bytesRemoved += oldLastObjSize - cit->second.size;
            } else {
                bytesAdded += cit->second.size - oldLastObjSize;
            }
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
                // expungeList.push_back(i.second.oid);
                newBlobSize -= i.first == oldLastOffset ? oldLastObjSize : i.second.size;
                bytesRemoved += (i.first == oldLastOffset) ? oldLastObjSize : i.second.size;  // Data Object size is *not* correct here for the last one. Surprise!
            }
        }

        objectsRemoved += truncateObjList.size();
    }

    // Is it possible to have an empty incoming metaList but with one or more offsets?
    // We certainly have hit a case in testing, and updateFwdCommittedBlob seems to think
    // it's possible.
    if (metaList != nullptr) {
        mergeMetaList(blobMeta.meta_list, *metaList);
    }
    blobMeta.desc.version += 1;
    blobMeta.desc.sequence_id = std::max(blobMeta.desc.sequence_id, seq_id);
    blobMeta.desc.blob_size = newBlobSize;  // Note that this is really *not* the blob size, neither logical or physical, due to "sparseness". Surprise!
    if (newBlob) {
        blobMeta.desc.blob_name = blobName;
    }

    rc = vol->putBatch(blobName, blobMeta, *blobObjList, delOffsetList);
    if (!rc.ok()) {
        LOGERROR << "Failed to put blob: '" << blobName << "' in volume: '" << std::hex
                << volId << std::dec << "' error: '" << rc << "'";
        return rc;
    }

    /**
     * We have a concern, before we apply these deltas, about whether the volume's
     * stat cache has been initialized (although given that AM must confirm that
     * all DM's within the volume's DM Redundancy group agree upon volume state before
     * doing anything with the volume - see the Consistency Spec - and so, if
     * implementation is according to spec - which, experience has shown that it may
     * not be - it may not be a "real" concern). The stat cache will have been
     * initialized if someone has called statVolumeLogical().
     *
     * To address the concern, we stat the volume ourselves if necessary. And doing so will capture
     * the "deltas" we just applied so that we do not have to explicitly apply them seperately.
     */
    if (!vol->volSummaryInitialized()) {
        fds_uint64_t size;
        fds_uint64_t blobs;
        fds_uint64_t objects;
        rc = statVolumeLogical(volId, &size, &blobs, &objects);
        if (!rc.ok()) {
            LOGERROR << "Failed to stat volume: '" << volId << "' error: '" << rc << "'";
            return rc;
        } else {
            LOGTRACE << "Initialized volume stats for volume ID <" << volId
                     << "> as blob count <" << blobs
                     << ">, logical data object count <" << objects
                     << ">, and logical byte size <" << size
                     << ">.";
        }
    } else {
        rc = vol->applyStatDeltas(bytesAdded, bytesRemoved,
                                  (newBlob ? 1 : 0) /* blobsAdded */, 0 /* blobsRemoved */,
                                  objectsAdded, objectsRemoved);
        if (!rc.ok()) {
            LOGERROR << "Failed to update volume stats: '" << volId << "' error: '" << rc << "'";
            return rc;
        } else {
            LOGTRACE << "Applied delta volume stats for volume ID <" << volId
                     << "> blobs added <" << (newBlob ? 1 : 0)
                     << "> blobs removed <" << 0
                     << ">, logical data objects added <" << objectsAdded
                     << ">, logical data objects removed <" << objectsRemoved
                     << ">, logical bytes added <" << bytesAdded
                     << ">, and logical bytes removed <" << bytesRemoved
                     << ">.";
        }
    }

    // actually expunge objects that were dereferenced by the blob
    // TODO(xxx): later that should become part of GC and done in background
    return ERR_OK;
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


    /**
     * We have a concern, before we apply these deltas, about whether the volume's
     * stat cache has been initialized (although given that AM must confirm that
     * all DM's within the volume's DM Redundancy group agree upon volume state before
     * doing anything with the volume - see the Consistency Spec - and so, if
     * implementation is according to spec - which, experience has shown that it may
     * not be - it may not be a "real" concern). The stat cache will have been
     * initialized if someone has called statVolumeLogical().
     *
     * To address the concern, we stat the volume ourselves if necessary. And doing so will capture
     * the "deltas" we just applied so that we do not have to explicitly apply them seperately.
     */
    if (!vol->volSummaryInitialized()) {
        fds_uint64_t size;
        fds_uint64_t blobs;
        fds_uint64_t objects;
        rc = statVolumeLogical(volId, &size, &blobs, &objects);
        if (!rc.ok()) {
            LOGERROR << "Failed to stat volume: '" << volId << "' error: '" << rc << "'";
            return rc;
        } else {
            LOGTRACE << "Initialized volume stats for volume ID <" << volId
                     << "> as blob count <" << blobs
                     << ">, logical data object count <" << objects
                     << ">, and logical byte size <" << size
                     << ">.";
        }
    } else {
        rc = vol->applyStatDeltas(newBlobSize, oldBlobSize,
                                  (newBlob ? 1 : 0) /* blobsAdded */, 0 /* blobsRemoved */,
                                  newObjCount, oldObjCount);
        if (!rc.ok()) {
            LOGERROR << "Failed to update volume stats: '" << volId << "' error: '" << rc << "'";
            return rc;
        } else {
            LOGTRACE << "Applied delta volume stats for volume ID <" << volId
                     << "> blobs added <" << (newBlob ? 1 : 0)
                     << "> blobs removed <" << 0
                     << ">, logical data objects added <" << newObjCount
                     << ">, logical data objects removed <" << oldObjCount
                     << ">, logical bytes added <" << newBlobSize
                     << ">, and logical bytes removed <" << oldBlobSize
                     << ">.";
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

        fds_uint64_t bytesRemoved = ((objList.size() - 1) * vol->getObjSize()) + lastObjectSize;
        fds_uint64_t objectsRemoved = objList.size();

        /**
         * We have a concern, before we apply these deltas, about whether the volume's
         * stat cache has been initialized (although given that AM must confirm that
         * all DM's within the volume's DM Redundancy group agree upon volume state before
         * doing anything with the volume - see the Consistency Spec - and so, if
         * implementation is according to spec - which, experience has shown that it may
         * not be - it may not be a "real" concern). The stat cache will have been
         * initialized if someone has called statVolumeLogical().
         *
         * To address the concern, we stat the volume ourselves if necessary. And doing so will capture
         * the "deltas" we just applied so that we do not have to explicitly apply them seperately.
         */
        if (!vol->volSummaryInitialized()) {
            fds_uint64_t size;
            fds_uint64_t blobs;
            fds_uint64_t objects;
            rc = statVolumeLogical(volId, &size, &blobs, &objects);
            if (!rc.ok()) {
                LOGERROR << "Failed to stat volume: '" << volId << "' error: '" << rc << "'";
                return rc;
            } else {
                LOGTRACE << "Initialized volume stats for volume ID <" << volId
                << "> as blob count <" << blobs
                << ">, logical data object count <" << objects
                << ">, and logical byte size <" << size
                << ">.";
            }
        } else {
            rc = vol->applyStatDeltas(0, bytesRemoved,
                                      0 /* blobsAdded */, 1 /* blobsRemoved */,
                                      0, objectsRemoved);
            if (!rc.ok()) {
                LOGERROR << "Failed to update volume stats: '" << volId << "' error: '" << rc << "'";
                return rc;
            } else {
                LOGTRACE << "Applied delta volume stats for volume ID <" << volId
                << "> blobs added <" << 0
                << "> blobs removed <" << 1
                << ">, logical data objects added <" << 0
                << ">, logical data objects removed <" << objectsRemoved
                << ">, logical bytes added <" << 0
                << ">, and logical bytes removed <" << bytesRemoved
                << ">.";
            }
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
                                         const std::string& blobData,
                                         VolumeMeta& volMeta){
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

    volMeta.setSequenceId(newBlob.desc.sequence_id);

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

        // if the new lastOffset is less than the olf lastOfffset, we need to truncate
        if (newLastOffset < oldLastOffset) {
            LOGDEBUG << "deleteObject start " << blobName << " newLastOffset: " << newLastOffset << " oldLastOffset: " << oldLastOffset;

            // delete starting at the offset AFTER the new last offset
            err = vol->deleteObject(blobName, newLastOffset + vol->getObjSize(), oldLastOffset);
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
                                                 Catalog::MemSnap snap,
                                                 const fds_bool_t &abortFlag) {

    GET_VOL_N_CHECK_DELETED(volId);
    return vol->getAllBlobsWithSequenceId(blobsSeqId, snap);

}

Error DmVolumeCatalog::getAllBlobsWithSequenceId(fds_volid_t volId, std::map<std::string, int64_t>& blobsSeqId,
                                                 Catalog::MemSnap snap) {
    fds_bool_t dummyFlag = false;
    return (getAllBlobsWithSequenceId(volId, blobsSeqId, snap, dummyFlag));
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

Error DmVolumeCatalog::getVersion(fds_volid_t volId, int32_t &version)
{
    GET_VOL_N_CHECK_DELETED(volId);
    version = vol->getVersion();
    return ERR_OK;
}

}  // namespace fds
