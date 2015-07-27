/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <map>
#include <vector>

#include <fds_process.h>
#include <fds_module.h>
#include <util/timeutils.h>
#include <util/path.h>

#include <dm-vol-cat/DmPersistVolDB.h>

#define TIMESTAMP_OP(WB) \
    const fds_uint64_t ts__ = util::getTimeStampMicros(); \
    const Record tsval__(reinterpret_cast<const char *>(&ts__), sizeof(fds_uint64_t)); \
    WB.Put(OP_TIMESTAMP_REC, tsval__);

namespace fds {

const std::string DmPersistVolDB::CATALOG_WRITE_BUFFER_SIZE_STR("catalog_write_buffer_size");
const std::string DmPersistVolDB::CATALOG_CACHE_SIZE_STR("catalog_cache_size");
const std::string DmPersistVolDB::CATALOG_MAX_LOG_FILES_STR("catalog_max_log_files");

Error status2error(leveldb::Status s){
    if (s.ok()) {
        return ERR_OK;
    }else if (s.IsCorruption()) {
        return ERR_ONDISK_DATA_CORRUPT;
    }else if (s.IsNotFound()) {
        return ERR_VOL_NOT_FOUND;
    }else if (s.IsIOError()) {
        return ERR_DISK_READ_FAILED;
    }

    return ERR_INVALID;
}


DmPersistVolDB::~DmPersistVolDB() {
    catalog_.reset();
    if (deleted_) {
        const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
        const std::string loc_src_db = (snapshot_ ? root->dir_user_repo_dm() :
                root->dir_sys_repo_dm()) + std::to_string(srcVolId_.get()) +
                (snapshot_ ? "/snapshot/" : "/") + getVolIdStr() + "_vcat.ldb";
        const std::string rm_cmd = "rm -rf  " + loc_src_db;
        int retcode = std::system((const char *)rm_cmd.c_str());
        LOGNOTIFY << "Removed leveldb dir, retcode " << retcode;
    }
}

Error DmPersistVolDB::activate() {
    const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
    std::string catName(snapshot_ ? root->dir_user_repo_dm() : root->dir_sys_repo_dm());
    if (!snapshot_ && srcVolId_ == invalid_vol_id) {
        // volume
        catName += getVolIdStr();
        catName += "/" + getVolIdStr() + "_vcat.ldb";
    } else if (srcVolId_ > invalid_vol_id && !snapshot_) {
        // clone
        catName += getVolIdStr();
        catName += "/" + getVolIdStr() + "_vcat.ldb";
    } else {
        // snapshot
        catName += std::to_string(srcVolId_.get()) + "/snapshot";
        catName += "/" + getVolIdStr() + "_vcat.ldb";
    }

    bool fAlreadyExists = util::dirExists(catName);
    if (snapshot_) {
        if (!fAlreadyExists) {
            LOGNORMAL << "Received activate on empty clone or snapshot! Directory " << catName;
            return ERR_OK;
        }
    }

    LOGNOTIFY << "Activating '" << catName << "'";
    FdsRootDir::fds_mkdir(catName.c_str());

    if (!snapshot_ && !readOnly_) {
        FdsRootDir::fds_mkdir(timelineDir_.c_str());
    }

    fds_uint32_t writeBufferSize = configHelper_.get<fds_uint32_t>(CATALOG_WRITE_BUFFER_SIZE_STR,
            Catalog::WRITE_BUFFER_SIZE);
    fds_uint32_t cacheSize = configHelper_.get<fds_uint32_t>(CATALOG_CACHE_SIZE_STR,
            Catalog::CACHE_SIZE);
    fds_uint32_t maxLogFiles = configHelper_.get<fds_uint32_t>(CATALOG_MAX_LOG_FILES_STR, 5);

    std::string logDirName = snapshot_ ? "" : root->dir_sys_repo_dm() + getVolIdStr() + "/";
    std::string logFilePrefix(snapshot_ ? "" : "catalog.journal");

    try
    {
        catalog_.reset(new Catalog(catName,
                                   writeBufferSize,
                                   cacheSize,
                                   logDirName,
                                   logFilePrefix,
                                   maxLogFiles,
                                   &cmp_));
    }
    catch(const CatalogException& e)
    {
        LOGERROR << "Failed to create catalog for volume " << std::hex << volId_ << std::dec;
        LOGERROR << e.what();
        if (fAlreadyExists) {
            LOGERROR << "unable to load existing vol:" << volId_ << " ...not activating";
            return ERR_DM_VOL_NOT_ACTIVATED;
        }
        return ERR_NOT_READY;
    }

    catalog_->GetWriteOptions().sync = false;

    // Write out the initial superblock descriptor into the volume
    fpi::FDSP_MetaDataList emptyMetadataList;
    VolumeMetaDesc volMetaDesc(emptyMetadataList, 0);
    if (ERR_OK != putVolumeMetaDesc(volMetaDesc)) {
        return ERR_DM_VOL_NOT_ACTIVATED;
    }

    activated_ = true;
    return ERR_OK;
}

Error DmPersistVolDB::copyVolDir(const std::string & destName) {
    return catalog_->DbSnap(destName);
}

Error DmPersistVolDB::getVolumeMetaDesc(VolumeMetaDesc & volDesc) {
    const BlobObjKey key(VOL_META_ID, 0);
    const Record keyRec(reinterpret_cast<const char *>(&key), sizeof(BlobObjKey));

    std::string value;
    Error rc = catalog_->Query(keyRec, &value);
    if (!rc.ok()) {
        LOGERROR << "Failed to get metadata volume: " << std::hex << volId_
                 << std::dec << "' error: '" << rc << "'";
        return rc;
    }
    return volDesc.loadSerialized(value);
}

Error DmPersistVolDB::getBlobMetaDesc(const std::string & blobName,
                                      BlobMetaDesc & blobMeta,
                                      Catalog::MemSnap snap) {
    const BlobObjKey key(DmPersistVolCat::getBlobIdFromName(blobName), BLOB_META_INDEX);
    const Record keyRec(reinterpret_cast<const char *>(&key), sizeof(BlobObjKey));

    std::string value;
    Error rc = catalog_->Query(keyRec, &value, snap);
    if (!rc.ok()) {
        LOGNOTIFY << "Failed to get metadata for blob: '" << blobName << "' volume: '"
                << std::hex << volId_ << std::dec << "' error: '" << rc << "'";
        return rc;
    }
    return blobMeta.loadSerialized(value);
}

Error DmPersistVolDB::getAllBlobMetaDesc(std::vector<BlobMetaDesc> & blobMetaList) {
    auto dbIt = catalog_->NewIterator();
    fds_assert(dbIt);
    for (dbIt->SeekToFirst(); dbIt->Valid(); dbIt->Next()) {
        Record dbKey = dbIt->key();
        if (reinterpret_cast<const BlobObjKey *>(dbKey.data())->objIndex == BLOB_META_INDEX) {
            BlobMetaDesc blobMeta;
            fds_verify(blobMeta.loadSerialized(dbIt->value().ToString()) == ERR_OK);
            blobMetaList.push_back(blobMeta);
        }
    }
    fds_assert(dbIt->status().ok());  // check for any errors during the scan

    return ERR_OK;
}

Error DmPersistVolDB::getAllBlobsWithSequenceId(std::map<std::string, int64_t>& blobsSeqId,
														Catalog::MemSnap snap) {
	auto dbIt = catalog_->NewIterator(snap);

	if (!dbIt) {
        LOGERROR << "Error generating set of <blobs,seqId> for volume: " << volId_;
        return ERR_INVALID;
    }

    for (dbIt->SeekToFirst(); dbIt->Valid(); dbIt->Next()) {
        Record dbKey = dbIt->key();
        if (reinterpret_cast<const BlobObjKey *>(dbKey.data())->objIndex == BLOB_META_INDEX) {
            BlobMetaDesc blobMeta;
            if (blobMeta.loadSerialized(dbIt->value().ToString()) != ERR_OK) {
                LOGERROR << "Error deserializing blob metadata when searching latest sequence id for volume " << volId_;
                return ERR_SERIALIZE_FAILED;
            }
            blobsSeqId.emplace(blobMeta.desc.blob_name, blobMeta.desc.sequence_id);
        }
    }

    if (!dbIt->status().ok()) {
        LOGERROR << "Error during generating set of blobs with sequence Ids for volume=" << volId_
                 << " with error=" << dbIt->status().ToString();
        return status2error(dbIt->status());
    }

	return (ERR_OK);
}

Error DmPersistVolDB::getLatestSequenceId(sequence_id_t & max) {
    auto dbIt = catalog_->NewIterator();
    if (!dbIt) {
        LOGERROR << "Error searching latest sequence id for volume " << volId_;
        return ERR_INVALID;
    }

    max = 0;

    for (dbIt->SeekToFirst(); dbIt->Valid(); dbIt->Next()) {
        Record dbKey = dbIt->key();
        if (reinterpret_cast<const BlobObjKey *>(dbKey.data())->objIndex == BLOB_META_INDEX) {
            BlobMetaDesc blobMeta;
            if (blobMeta.loadSerialized(dbIt->value().ToString()) != ERR_OK) {
                LOGERROR << "Error deserializing blob metadata when searching latest sequence id for volume " << volId_;
                return ERR_SERIALIZE_FAILED;
            }

            if (max < blobMeta.desc.sequence_id) {
                max = blobMeta.desc.sequence_id;
            }
        }
    }

    Error err;
    if (dbIt->status().ok()) {
        fpi::FDSP_MetaDataList emptyMetadataList;
        VolumeMetaDesc volMetaDesc(emptyMetadataList, 0);

        err = getVolumeMetaDesc(volMetaDesc);

        if (err.ok()) {
            if (max < volMetaDesc.sequence_id) {
                max = volMetaDesc.sequence_id;
            }
        } else {
            LOGERROR << "Error searching volume descriptor for latest sequence id for volume "
                     << volId_ << ": " << err;
        }
    } else {
        LOGERROR << "Error searching latest sequence id for volume " << volId_
                 << ": " << dbIt->status().ToString();

        err = status2error(dbIt->status());
    }

    return err;
}

Error DmPersistVolDB::getObject(const std::string & blobName, fds_uint64_t offset,
        ObjectID & obj) {
    fds_verify(0 == offset % objSize_);

    const BlobObjKey key(DmPersistVolCat::getBlobIdFromName(blobName), offset / objSize_);
    const Record keyRec(reinterpret_cast<const char *>(&key), sizeof(BlobObjKey));

    std::string value;

    Error rc = catalog_->Query(keyRec, &value);
    if (!rc.ok()) {
        LOGNOTIFY << "Failed to get oid for offset: '" << std::hex << offset << std::dec
                << "' blob: '" << blobName << "' volume: '" << std::hex << volId_ <<
                std::dec << "'";
        return rc;
    }

    obj.SetId(value);
    return rc;
}

Error DmPersistVolDB::getObject(const std::string & blobName, fds_uint64_t startOffset,
                                fds_uint64_t endOffset, fpi::FDSP_BlobObjectList& objList,
                                const Catalog::MemSnap snap) {
    fds_assert(startOffset <= endOffset);
    fds_verify(0 == startOffset % objSize_);
    fds_verify(0 == endOffset % objSize_);

    fds_uint64_t blobId = DmPersistVolCat::getBlobIdFromName(blobName);
    fds_uint32_t startObjIndex = startOffset / objSize_;
    fds_uint32_t endObjIndex = endOffset / objSize_;

    BlobObjKey startKey(blobId, startObjIndex);
    const Record startRec(reinterpret_cast<const char *>(&startKey), sizeof(BlobObjKey));

    BlobObjKey endKey(blobId, endObjIndex);
    const Record endRec(reinterpret_cast<const char *>(&endKey), sizeof(BlobObjKey));

    auto dbIt = catalog_->NewIterator(snap);

    if (!dbIt) {
        LOGERROR << "Error creating iterator for ldb on volume " << volId_;
        return ERR_INVALID;
    }

    objList.reserve(endObjIndex - startObjIndex + 1);
    for (dbIt->Seek(startRec); dbIt->Valid() &&
            catalog_->GetOptions().comparator->Compare(dbIt->key(), endRec) <= 0;
            dbIt->Next()) {
        const BlobObjKey * key = reinterpret_cast<const BlobObjKey *>(dbIt->key().data());

        fpi::FDSP_BlobObjectInfo blobInfo;
        blobInfo.offset = static_cast<fds_uint64_t>(key->objIndex) * objSize_;
        blobInfo.size = objSize_;
        blobInfo.data_obj_id.digest = std::move(dbIt->value().ToString());

        objList.push_back(std::move(blobInfo));
    }

    if (!dbIt->status().ok()) {
        LOGERROR << "Error getting offsets for blob " << blobName << " for volume " << volId_
                 << " : " << dbIt->status().ToString();

        return status2error(dbIt->status());
    }

    return ERR_OK;
}

Error DmPersistVolDB::getObject(const std::string & blobName, fds_uint64_t startOffset,
        fds_uint64_t endOffset, BlobObjList & objList) {
    fds_assert(startOffset <= endOffset);
    fds_verify(0 == startOffset % objSize_);
    fds_verify(0 == endOffset % objSize_);

    fds_uint64_t blobId = DmPersistVolCat::getBlobIdFromName(blobName);
    fds_uint32_t startObjIndex = startOffset / objSize_;
    fds_uint32_t endObjIndex = endOffset / objSize_;

    BlobObjKey startKey(blobId, startObjIndex);
    const Record startRec(reinterpret_cast<const char *>(&startKey), sizeof(BlobObjKey));

    BlobObjKey endKey(blobId, endObjIndex);
    const Record endRec(reinterpret_cast<const char *>(&endKey), sizeof(BlobObjKey));

    auto dbIt = catalog_->NewIterator();
    fds_assert(dbIt);
    for (dbIt->Seek(startRec); dbIt->Valid() &&
            catalog_->GetOptions().comparator->Compare(dbIt->key(), endRec) <= 0;
            dbIt->Next()) {
        const BlobObjKey * key = reinterpret_cast<const BlobObjKey *>(dbIt->key().data());

        BlobObjInfo blobInfo;
        blobInfo.size = getObjSize();
        blobInfo.oid.SetId(dbIt->value().data(), dbIt->value().size());

        objList[static_cast<fds_uint64_t>(key->objIndex) * objSize_] = blobInfo;
    }
    fds_assert(dbIt->status().ok());  // check for any errors during the scan

    return ERR_OK;
}

Error DmPersistVolDB::putVolumeMetaDesc(const VolumeMetaDesc & volDesc) {
    const BlobObjKey key(VOL_META_ID, 0);
    const Record keyRec(reinterpret_cast<const char *>(&key), sizeof(BlobObjKey));

    std::string value;
    Error rc = volDesc.getSerialized(value);
    if (rc.ok()) {
        CatWriteBatch batch;
        TIMESTAMP_OP(batch);
        batch.Put(keyRec, value);
        rc = catalog_->Update(&batch);
        if (!rc.ok()) {
            LOGERROR << "Failed to update metadata descriptor for volume: "
                     << volDesc;
        } else {
            LOGNORMAL << "Successfully updated metadata descriptor for volume: "
                      << volDesc;
        }
    }

    return rc;
}

Error DmPersistVolDB::putBlobMetaDesc(const std::string & blobName,
        const BlobMetaDesc & blobMeta) {
    IS_OP_ALLOWED();

    const BlobObjKey key(DmPersistVolCat::getBlobIdFromName(blobName), BLOB_META_INDEX);
    const Record keyRec(reinterpret_cast<const char *>(&key), sizeof(BlobObjKey));

    std::string value;
    Error rc = blobMeta.getSerialized(value);
    if (rc.ok()) {
        CatWriteBatch batch;
        TIMESTAMP_OP(batch);
        batch.Put(keyRec, value);
        rc = catalog_->Update(&batch);
        if (!rc.ok()) {
            LOGERROR << "Failed to update metadata for blob: '" << blobName << "' volume: '"
                    << std::hex << volId_ << std::dec << "'";
        }
    }

    return rc;
}

Error DmPersistVolDB::putObject(const std::string & blobName, fds_uint64_t offset,
        const ObjectID & obj) {
    IS_OP_ALLOWED();

    fds_verify(0 == offset % objSize_);

    const BlobObjKey key(DmPersistVolCat::getBlobIdFromName(blobName), offset / objSize_);
    const Record keyRec(reinterpret_cast<const char *>(&key), sizeof(BlobObjKey));
    const Record valRec(reinterpret_cast<const char *>(obj.GetId()), obj.GetLen());

    CatWriteBatch batch;
    TIMESTAMP_OP(batch);
    batch.Put(keyRec, valRec);
    {
        return catalog_->Update(&batch);
    }
}

Error DmPersistVolDB::putObject(const std::string & blobName, const BlobObjList & objs) {
    IS_OP_ALLOWED();

    Error rc = ERR_OK;
    if (objs.empty()) {
        return rc;
    }

    fds_uint64_t blobId = DmPersistVolCat::getBlobIdFromName(blobName);

    CatWriteBatch batch;
    TIMESTAMP_OP(batch);
    for (auto & it : objs) {
        fds_verify(0 == it.first % objSize_);
        const BlobObjKey key(blobId, it.first / objSize_);
        const Record keyRec(reinterpret_cast<const char *>(&key), sizeof(BlobObjKey));
        const Record valRec(reinterpret_cast<const char *>(it.second.oid.GetId()),
                it.second.oid.GetLen());

        batch.Put(keyRec, valRec);
    }

    rc = catalog_->Update(&batch);
    if (!rc.ok()) {
        LOGERROR << "Failed to put blob: '" << blobName << "' volume: '" << std::hex
                << volId_ << std::dec << "'";
    }

    return rc;
}

Error DmPersistVolDB::putBatch(const std::string & blobName, const BlobMetaDesc & blobMeta,
        const BlobObjList & puts, const std::vector<fds_uint64_t> & deletes) {
    IS_OP_ALLOWED();

    fds_uint64_t blobId = DmPersistVolCat::getBlobIdFromName(blobName);
    CatWriteBatch batch;
    TIMESTAMP_OP(batch);

    for (auto & it : puts) {
        fds_verify(0 == it.first % objSize_);
        const BlobObjKey key(blobId, it.first / objSize_);
        const Record keyRec(reinterpret_cast<const char *>(&key), sizeof(BlobObjKey));
        const Record valRec(reinterpret_cast<const char *>(it.second.oid.GetId()),
                it.second.oid.GetLen());

        batch.Put(keyRec, valRec);
    }

    for (auto & it : deletes) {
        fds_verify(0 == it % objSize_);
        const BlobObjKey key(blobId, it / objSize_);
        const Record keyRec(reinterpret_cast<const char *>(&key), sizeof(BlobObjKey));
        batch.Delete(keyRec);
    }

    const BlobObjKey key(blobId, BLOB_META_INDEX);
    const Record keyRec(reinterpret_cast<const char *>(&key), sizeof(BlobObjKey));

    std::string value;
    Error rc = blobMeta.getSerialized(value);
    if (!rc.ok()) {
        LOGERROR << "Failed to update metadata for blob: '" << blobName << "' volume: '"
                << std::hex << volId_ << std::dec << "'";
        return rc;
    }

    batch.Put(keyRec, value);
    rc = catalog_->Update(&batch);
    if (!rc.ok()) {
        LOGERROR << "Failed to put blob: '" << blobName << "' volume: '" << std::hex
                << volId_ << std::dec << "'";
    }

    return rc;
}

Error DmPersistVolDB::putBatch(const std::string & blobName, const BlobMetaDesc & blobMeta,
            CatWriteBatch & wb) {
    IS_OP_ALLOWED();

    TIMESTAMP_OP(wb);

    const BlobObjKey key(DmPersistVolCat::getBlobIdFromName(blobName), BLOB_META_INDEX);
    const Record keyRec(reinterpret_cast<const char *>(&key), sizeof(BlobObjKey));

    std::string value;
    Error rc = blobMeta.getSerialized(value);
    if (!rc.ok()) {
        LOGERROR << "Failed to update metadata for blob: '" << blobName << "' volume: '"
                << std::hex << volId_ << std::dec << "'";
        return rc;
    }

    wb.Put(keyRec, value);
    rc = catalog_->Update(&wb);
    if (!rc.ok()) {
        LOGERROR << "Failed to put blob: '" << blobName << "' volume: '" << std::hex
                << volId_ << std::dec << "'";
    }

    return rc;
}

Error DmPersistVolDB::deleteObject(const std::string & blobName, fds_uint64_t offset) {
    // IS_OP_ALLOWED();

    fds_uint64_t blobId = DmPersistVolCat::getBlobIdFromName(blobName);
    const BlobObjKey key(blobId, offset / objSize_);
    const Record keyRec(reinterpret_cast<const char *>(&key), sizeof(BlobObjKey));

    CatWriteBatch batch;
    TIMESTAMP_OP(batch);
    batch.Delete(keyRec);
    Error rc = catalog_->Update(&batch);
    if (!rc.ok()) {
        LOGERROR << "Failed to delete object at offset '" << std::hex << offset << std::dec
                << "' of a blob: '" << blobName << "' volume: '" << std::hex << volId_ <<
                std::dec << "'";
    }
    return rc;
}

Error DmPersistVolDB::deleteObject(const std::string & blobName, fds_uint64_t startOffset,
        fds_uint64_t endOffset) {
    // commenting this out to support snapshot delete
    // IS_OP_ALLOWED();

    fds_uint64_t blobId = DmPersistVolCat::getBlobIdFromName(blobName);
    CatWriteBatch batch;
    TIMESTAMP_OP(batch);
    for (fds_uint64_t i = startOffset; i <= endOffset; i += objSize_) {
        const BlobObjKey key(blobId, i / objSize_);
        const Record keyRec(reinterpret_cast<const char *>(&key), sizeof(BlobObjKey));
        batch.Delete(keyRec);
    }

    Error rc = catalog_->Update(&batch);
    if (!rc.ok()) {
        LOGERROR << "Failed to delete object for blob: '" << blobName << "' volume: '"
                << std::hex << volId_ << std::dec << "'";
    }
    return rc;
}

Error DmPersistVolDB::deleteBlobMetaDesc(const std::string & blobName) {
    //IS_OP_ALLOWED();

    LOGDEBUG << "Deleting metadata for blob: '" << blobName << "' volume: '" << std::hex
             << volId_ << std::dec << "'";

    const BlobObjKey key(DmPersistVolCat::getBlobIdFromName(blobName), BLOB_META_INDEX);
    const Record keyRec(reinterpret_cast<const char *>(&key), sizeof(BlobObjKey));

    CatWriteBatch batch;
    TIMESTAMP_OP(batch);
    batch.Delete(keyRec);
    return catalog_->Update(&batch);
}

Error DmPersistVolDB::getInMemorySnapshot(Catalog::MemSnap &snap) {
    catalog_->GetSnapshot(snap);
    return ERR_OK;
}

Error DmPersistVolDB::freeInMemorySnapshot(Catalog::MemSnap snap)  {
    catalog_->ReleaseSnapshot(snap);
    return ERR_OK;
}

void DmPersistVolDB::forEachObject(std::function<void(const ObjectID&)> func) {
    auto dbIt = catalog_->NewIterator();
    Error err;
    ObjectID objId;
    fds_assert(dbIt);
    for (dbIt->SeekToFirst(); dbIt->Valid(); dbIt->Next()) {
        Record dbKey = dbIt->key();
        const BlobObjKey *blobKey = reinterpret_cast<const BlobObjKey *>(dbKey.data());
        if (blobKey->blobId != VOL_META_ID &&
            blobKey->objIndex !=BLOB_META_INDEX &&
            blobKey->blobId != INVALID_BLOB_ID) {
            objId.SetId(dbIt->value().ToString());
            // LOGDEBUG << "blobid:" << blobKey->blobId << " idx:" << blobKey->objIndex << " " << objId;
            func(objId);
        }
    }
    fds_assert(dbIt->status().ok());  // check for any errors during the scan
}
}  // namespace fds
