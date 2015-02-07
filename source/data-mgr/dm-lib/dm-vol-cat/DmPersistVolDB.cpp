/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>
#include <map>
#include <vector>

#include <fds_process.h>
#include <fds_module.h>
#include <util/timeutils.h>

#include <dm-vol-cat/DmPersistVolDB.h>

#define TIMESTAMP_OP(WB) \
    const fds_uint64_t ts__ = util::getTimeStampMicros(); \
    const Record tsval__(reinterpret_cast<const char *>(&ts__), sizeof(fds_uint64_t)); \
    WB.Put(OP_TIMESTAMP_REC, tsval__);

namespace fds {

const std::string DmPersistVolDB::CATALOG_WRITE_BUFFER_SIZE_STR("catalog_write_buffer_size");
const std::string DmPersistVolDB::CATALOG_CACHE_SIZE_STR("catalog_cache_size");
const std::string DmPersistVolDB::CATALOG_MAX_LOG_FILES_STR("catalog_max_log_files");

DmPersistVolDB::~DmPersistVolDB() {
    delete catalog_;

    if (deleted_) {
        const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
        const std::string loc_src_db = root->dir_user_repo_dm() + std::to_string(srcVolId_) +
                (snapshot_ ? "/snapshot/" : "/") + getVolIdStr() + "_vcat.ldb";
        const std::string rm_cmd = "rm -rf  " + loc_src_db;
        int retcode = std::system((const char *)rm_cmd.c_str());
        LOGNOTIFY << "Removed leveldb dir, retcode " << retcode;
    }
}

Error DmPersistVolDB::activate() {
    const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
    std::string catName(root->dir_user_repo_dm());
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
        catName += std::to_string(srcVolId_) + "/snapshot";
        catName += "/" + getVolIdStr() + "_vcat.ldb";
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

    std::string logDirName = snapshot_ ? "" : root->dir_user_repo_dm() + getVolIdStr() + "/";
    std::string logFilePrefix(snapshot_ ? "" : "catalog.journal");

    try
    {
        if (catalog_) delete catalog_;
        catalog_ = new Catalog(catName, writeBufferSize, cacheSize, logDirName,
                    logFilePrefix, maxLogFiles, &cmp_);
    }
    catch(const CatalogException& e)
    {
        LOGERROR << "Failed to create catalog for volume " << std::hex << volId_ << std::dec;
        LOGERROR << e.what();
        return ERR_NOT_READY;
    }

    catalog_->GetWriteOptions().sync = false;

    activated_ = true;
    return ERR_OK;
}

Error DmPersistVolDB::copyVolDir(const std::string & destName) {
    return catalog_->DbSnap(destName);
}

Error DmPersistVolDB::getBlobMetaDesc(const std::string & blobName,
        BlobMetaDesc & blobMeta) {
    const BlobObjKey key(DmPersistVolCat::getBlobIdFromName(blobName), BLOB_META_INDEX);
    const Record keyRec(reinterpret_cast<const char *>(&key), sizeof(BlobObjKey));

    std::string value;
    Error rc = catalog_->Query(keyRec, &value);
    if (!rc.ok()) {
        LOGNOTIFY << "Failed to get metadata for blob: '" << blobName << "' volume: '"
                << std::hex << volId_ << std::dec << "' error: '" << rc << "'";
        return rc;
    }
    return blobMeta.loadSerialized(value);
}

Error DmPersistVolDB::getAllBlobMetaDesc(std::vector<BlobMetaDesc> & blobMetaList) {
    Catalog::catalog_roptions_t opts;
    Catalog::catalog_iterator_t * dbIt = getSnapshotIter(opts);
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
    delete dbIt;

    return ERR_OK;
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
        fds_uint64_t endOffset, fpi::FDSP_BlobObjectList& objList) {
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

    Catalog::catalog_iterator_t * dbIt = catalog_->NewIterator();
    fds_assert(dbIt);
    objList.reserve(endObjIndex - startObjIndex + 1);
    for (dbIt->Seek(startRec); dbIt->Valid() &&
            catalog_->GetOptions().comparator->Compare(dbIt->key(), endRec) <= 0;
            dbIt->Next()) {
        const BlobObjKey * key = reinterpret_cast<const BlobObjKey *>(dbIt->key().data());

        fpi::FDSP_BlobObjectInfo blobInfo;
        blobInfo.offset = static_cast<fds_uint64_t>(key->objIndex) * objSize_;
        blobInfo.size = objSize_;
        blobInfo.blob_end = false;  // assume false
        blobInfo.data_obj_id.digest = std::move(dbIt->value().ToString());

        objList.push_back(std::move(blobInfo));
    }
    fds_assert(dbIt->status().ok());  // check for any errors during the scan
    delete dbIt;

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

    Catalog::catalog_iterator_t * dbIt = catalog_->NewIterator();
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
    delete dbIt;

    return ERR_OK;
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
    return catalog_->Update(&batch);
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
    IS_OP_ALLOWED();

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
    IS_OP_ALLOWED();

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
    IS_OP_ALLOWED();

    LOGDEBUG << "Deleting metadata for blob: '" << blobName << "' volume: '" << std::hex
            << volId_ << std::dec << "'";

    const BlobObjKey key(DmPersistVolCat::getBlobIdFromName(blobName), BLOB_META_INDEX);
    const Record keyRec(reinterpret_cast<const char *>(&key), sizeof(BlobObjKey));

    CatWriteBatch batch;
    TIMESTAMP_OP(batch);
    batch.Delete(keyRec);
    return catalog_->Update(&batch);
}
}  // namespace fds
