/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

// Standard includes.
#include <catalogKeys/BlobMetadataKey.h>
#include <catalogKeys/BlobObjectKey.h>
#include <catalogKeys/VolumeMetadataKey.h>
#include <limits>
#include <map>
#include <string>
#include <vector>

// Internal includes.
#include "catalogKeys/CatalogKeyComparator.h"
#include "catalogKeys/CatalogKeyType.h"
#include "dm-vol-cat/DmPersistVolDB.h"
#include "leveldb/db.h"
#include <util/stringutils.h>
#include "util/timeutils.h"
#include "util/path.h"
#include "fds_module.h"
#include "fds_process.h"
#include <net/volumegroup_extensions.h>

#define TIMESTAMP_OP(WB) \
    const fds_uint64_t ts__ = util::getTimeStampMicros(); \
    const leveldb::Slice tsval__(reinterpret_cast<const char *>(&ts__), sizeof(fds_uint64_t)); \
    WB.Put(OP_TIMESTAMP_REC, tsval__);

namespace fds {

const std::string DmPersistVolDB::CATALOG_WRITE_BUFFER_SIZE_STR("catalog_write_buffer_size");
const std::string DmPersistVolDB::CATALOG_CACHE_SIZE_STR("catalog_cache_size");
const std::string DmPersistVolDB::CATALOG_MAX_LOG_FILES_STR("catalog_max_log_files");
const std::string DmPersistVolDB::ENABLE_TIMELINE_STR("enable_timeline");

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
        const FdsRootDir* root = MODULEPROVIDER()->proc_fdsroot();
        const std::string loc_src_db = (snapshot_ ? root->dir_user_repo_dm() :
                root->dir_sys_repo_dm()) + std::to_string(srcVolId_.get()) +
                (snapshot_ ? "/snapshot/" : "/") + getVolIdStr() + "_vcat.ldb";
        const std::string rm_cmd = "rm -rf  " + loc_src_db;
        int retcode = std::system((const char *)rm_cmd.c_str());
        LOGNOTIFY << "Removed leveldb dir, retcode " << retcode;
    }
}

uint64_t DmPersistVolDB::getNumInMemorySnapshots() {
    return snapshotCount.load(std::memory_order_relaxed);
}

Error DmPersistVolDB::activate() {
    const FdsRootDir* root = MODULEPROVIDER()->proc_fdsroot();
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
    fds_bool_t timelineEnable = configHelper_.get<fds_bool_t>(ENABLE_TIMELINE_STR,false);

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
                                   timelineEnable,
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

    /* Update version */
    if (!snapshot_) {
        /* Read, increment, and persist new version */
        int32_t version = getVersion();
        if (version == VolumeGroupConstants::VERSION_INVALID) {
            version = VolumeGroupConstants::VERSION_START;
        } else {
            version++;
        }
        setVersion(version);
    }

    activated_ = true;
    return ERR_OK;
}

Error DmPersistVolDB::copyVolDir(const std::string & destName) {
    return catalog_->DbSnap(destName);
}

Error DmPersistVolDB::getVolumeMetaDesc(VolumeMetaDesc & volDesc) {
    VolumeMetadataKey const key;

    std::string value;
    Error rc = catalog_->Query(key, &value);
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
    BlobMetadataKey key {blobName};

    std::string value;
    Error rc = catalog_->Query(key, &value, snap);
    if (!rc.ok()) {
        if (rc != ERR_CAT_ENTRY_NOT_FOUND) {
            LOGERROR << "Failed to get metadata for blob: '" << blobName << "' volume: '"
                     << std::hex << volId_ << std::dec << "' error: '" << rc << "'";
        }
        return rc;
    }
    return blobMeta.loadSerialized(value);
}

Error DmPersistVolDB::getAllBlobMetaDesc(std::vector<BlobMetaDesc> & blobMetaList) {
    auto dbIt = catalog_->NewIterator();
    fds_assert(dbIt);
    for (dbIt->SeekToFirst(); dbIt->Valid(); dbIt->Next()) {
        fds_assert(dbIt->status().ok());
        leveldb::Slice dbKey = dbIt->key();
        if (*reinterpret_cast<CatalogKeyType const*>(dbKey.data()) == CatalogKeyType::BLOB_METADATA) {
            BlobMetaDesc blobMeta;
            fds_verify(blobMeta.loadSerialized(dbIt->value().ToString()) == ERR_OK);
            blobMetaList.push_back(blobMeta);
        }
    }

    return ERR_OK;
}

Error DmPersistVolDB::getBlobMetaDescForPrefix (std::string const& prefix,
                                                std::string const& delimiter,
                                                std::vector<BlobMetaDesc>& blobMetaList,
                                                std::vector<std::string>& skippedPrefixes)
{
    if (prefix.empty() && delimiter.empty())
    {
        return getAllBlobMetaDesc(blobMetaList);
    }
    else
    {
        auto dbIt = catalog_->NewIterator();
        fds_assert(dbIt);

        auto& catalogOptions = catalog_->GetOptions();
        auto& comparator = *catalogOptions.comparator;
        auto& typedComparator = dynamic_cast<CatalogKeyComparator const&>(comparator);

        BlobMetadataKey begin { prefix };
        BlobMetadataKey end { typedComparator.getIncremented(begin) };

        auto beginSlice { static_cast<leveldb::Slice>(begin) };
        auto endSlice { static_cast<leveldb::Slice>(end) };

        for (prefix.empty() ? dbIt->SeekToFirst() : dbIt->Seek(begin);
             dbIt->Valid() && (prefix.empty() || comparator.Compare(dbIt->key(), end) < 0);
             dbIt->Next())
        {
            fds_assert(dbIt->status().ok());

            BlobMetaDesc blobMetadata;
            fds_verify(blobMetadata.loadSerialized(dbIt->value().ToString()) == ERR_OK);

            if (!delimiter.empty())
            {
                auto& blobName = blobMetadata.desc.blob_name;
                auto delimiterPosition = blobName.find(delimiter, prefix.size());
                if (delimiterPosition != std::string::npos)
                {
                    auto blobNameToDelimiter = blobName.substr(0, delimiterPosition + 1);
                    BlobMetadataKey blobNameToDelimiterEnd { blobNameToDelimiter };

                    dbIt->Seek(typedComparator.getIncremented(blobNameToDelimiterEnd));

                    skippedPrefixes.push_back(blobNameToDelimiter.substr(prefix.size()));
                    continue;
                }
            }

            blobMetaList.push_back(blobMetadata);
        }

        return ERR_OK;
    }
}

Error DmPersistVolDB::getAllBlobsWithSequenceId(std::map<std::string, int64_t>& blobsSeqId,
														Catalog::MemSnap snap) {
    fds_bool_t dummyFlag = false;
    return (getAllBlobsWithSequenceId(blobsSeqId, snap, dummyFlag));
}

Error DmPersistVolDB::getAllBlobsWithSequenceId(std::map<std::string, int64_t>& blobsSeqId,
														Catalog::MemSnap snap,
														const fds_bool_t &abortFlag) {
	auto dbIt = catalog_->NewIterator(snap);

	if (!dbIt) {
        LOGERROR << "Error generating set of <blobs,seqId> for volume: " << volId_;
        return ERR_INVALID;
    }

    for (dbIt->SeekToFirst(); dbIt->Valid(); dbIt->Next()) {
        // Check to see if we are called to stop
        if (abortFlag) {
            LOGDEBUG << "Abort migration called. Exiting catalog operations.";
            return (ERR_DM_MIGRATION_ABORTED);
        }

        leveldb::Slice dbKey = dbIt->key();
        if (*reinterpret_cast<CatalogKeyType const*>(dbKey.data()) == CatalogKeyType::BLOB_METADATA) {
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
        leveldb::Slice dbKey = dbIt->key();
        if (*reinterpret_cast<CatalogKeyType const*>(dbKey.data()) == CatalogKeyType::BLOB_METADATA) {
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

    auto objectIndex = offset / objSize_;
    fds_verify(objectIndex <= std::numeric_limits<fds_uint32_t>::max());

    BlobObjectKey key {blobName, static_cast<fds_uint32_t>(objectIndex)};

    std::string value;

    Error rc = catalog_->Query(key, &value);
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

    fds_uint32_t startObjIndex = startOffset / objSize_;
    fds_uint32_t endObjIndex = endOffset / objSize_;

    BlobObjectKey startKey {blobName, startObjIndex};
    BlobObjectKey endKey {blobName, endObjIndex};
    leveldb::Slice endSlice {static_cast<leveldb::Slice>(endKey)};

    auto dbIt = catalog_->NewIterator(snap);

    if (!dbIt) {
        LOGERROR << "Error creating iterator for ldb on volume " << volId_;
        return ERR_INVALID;
    }

    objList.reserve(endObjIndex - startObjIndex + 1);
    for (dbIt->Seek(static_cast<leveldb::Slice>(startKey));
         dbIt->Valid()
         && catalog_->GetOptions().comparator->Compare(dbIt->key(), endSlice) <= 0;
         dbIt->Next()) {
        BlobObjectKey key { dbIt->key() };

        fpi::FDSP_BlobObjectInfo blobInfo;
        blobInfo.offset = static_cast<fds_uint64_t>(key.getObjectIndex()) * objSize_;
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

    fds_uint32_t startObjIndex = startOffset / objSize_;
    fds_uint32_t endObjIndex = endOffset / objSize_;

    BlobObjectKey startKey {blobName, startObjIndex};
    BlobObjectKey endKey {blobName, endObjIndex};
    leveldb::Slice endSlice {static_cast<leveldb::Slice>(endKey)};

    auto dbIt = catalog_->NewIterator();
    fds_assert(dbIt);
    for (dbIt->Seek(static_cast<leveldb::Slice>(startKey));
         dbIt->Valid()
         && catalog_->GetOptions().comparator->Compare(dbIt->key(), endSlice) <= 0;
         dbIt->Next()) {
        BlobObjectKey key { dbIt->key() };

        BlobObjInfo blobInfo;
        blobInfo.size = getObjSize();
        blobInfo.oid.SetId(dbIt->value().data(), dbIt->value().size());

        objList[static_cast<fds_uint64_t>(key.getObjectIndex()) * objSize_] = blobInfo;
    }
    fds_assert(dbIt->status().ok());  // check for any errors during the scan

    return ERR_OK;
}

Error DmPersistVolDB::putVolumeMetaDesc(const VolumeMetaDesc & volDesc) {
    VolumeMetadataKey key;

    std::string value;
    Error rc = volDesc.getSerialized(value);
    if (rc.ok()) {
        CatWriteBatch batch;
        TIMESTAMP_OP(batch);
        batch.Put(static_cast<leveldb::Slice>(key), value);
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

    BlobMetadataKey key {blobName};

    std::string value;
    Error rc = blobMeta.getSerialized(value);
    if (rc.ok()) {
        CatWriteBatch batch;
        TIMESTAMP_OP(batch);
        batch.Put(static_cast<leveldb::Slice>(key), value);
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

    auto objectIndex = offset / objSize_;
    fds_verify(objectIndex <= std::numeric_limits<fds_uint32_t>::max());

    BlobObjectKey key {blobName, static_cast<fds_uint32_t>(objectIndex)};
    leveldb::Slice const valRec(reinterpret_cast<char const*>(obj.GetId()), obj.GetLen());

    CatWriteBatch batch;
    TIMESTAMP_OP(batch);
    batch.Put(static_cast<leveldb::Slice>(key), valRec);
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

    CatWriteBatch batch;
    TIMESTAMP_OP(batch);
    for (auto & it : objs) {
        fds_verify(0 == it.first % objSize_);

        auto objectIndex = it.first / objSize_;
        fds_verify(objectIndex <= std::numeric_limits<fds_uint32_t>::max());

        BlobObjectKey const key {blobName, static_cast<fds_uint32_t>(objectIndex)};
        leveldb::Slice const valRec(reinterpret_cast<const char *>(it.second.oid.GetId()),
                                    it.second.oid.GetLen());

        batch.Put(static_cast<leveldb::Slice>(key), valRec);
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

    CatWriteBatch batch;
    TIMESTAMP_OP(batch);

    for (auto & it : puts) {
        fds_verify(0 == it.first % objSize_);

        auto objectIndex = it.first / objSize_;
        fds_verify(objectIndex <= std::numeric_limits<fds_uint32_t>::max());

        BlobObjectKey const key {blobName, static_cast<fds_uint32_t>(objectIndex)};
        leveldb::Slice const valRec(reinterpret_cast<const char *>(it.second.oid.GetId()),
                                    it.second.oid.GetLen());

        batch.Put(static_cast<leveldb::Slice>(key), valRec);
    }

    for (auto & it : deletes) {
        fds_verify(0 == it % objSize_);

        auto objectIndex = it / objSize_;
        fds_verify(objectIndex <= std::numeric_limits<fds_uint32_t>::max());

        BlobObjectKey const key {blobName, static_cast<fds_uint32_t>(objectIndex)};
        batch.Delete(static_cast<leveldb::Slice>(key));
    }

    BlobMetadataKey const key {blobName};

    std::string value;
    Error rc = blobMeta.getSerialized(value);
    if (!rc.ok()) {
        LOGERROR << "Failed to update metadata for blob: '" << blobName << "' volume: '"
                << std::hex << volId_ << std::dec << "'";
        return rc;
    }

    batch.Put(static_cast<leveldb::Slice>(key), value);
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

    BlobMetadataKey const key {blobName};

    std::string value;
    Error rc = blobMeta.getSerialized(value);
    if (!rc.ok()) {
        LOGERROR << "Failed to update metadata for blob: '" << blobName << "' volume: '"
                << std::hex << volId_ << std::dec << "'";
        return rc;
    }

    wb.Put(static_cast<leveldb::Slice>(key), value);
    rc = catalog_->Update(&wb);
    if (!rc.ok()) {
        LOGERROR << "Failed to put blob: '" << blobName << "' volume: '" << std::hex
                << volId_ << std::dec << "'";
    }

    return rc;
}

Error DmPersistVolDB::deleteObject(const std::string & blobName, fds_uint64_t offset) {
    // IS_OP_ALLOWED();

    auto objectIndex = offset / objSize_;
    fds_verify(objectIndex <= std::numeric_limits<fds_uint32_t>::max());

    BlobObjectKey const key {blobName, static_cast<fds_uint32_t>(objectIndex)};

    CatWriteBatch batch;
    TIMESTAMP_OP(batch);
    batch.Delete(static_cast<leveldb::Slice>(key));
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

    BlobObjectKey key {blobName};
    Error rc(ERR_OK);

    unsigned counter = 0;
    for (fds_uint64_t i = startOffset; i <= endOffset; i += objSize_) {
        // For now, flush each objSize. This should prevent gigantic delete batch
        CatWriteBatch batch;
        TIMESTAMP_OP(batch);

        auto objectIndex = i / objSize_;
        fds_verify(objectIndex <= std::numeric_limits<fds_uint32_t>::max());

        key.setObjectIndex(static_cast<fds_uint32_t>(objectIndex));
        batch.Delete(static_cast<leveldb::Slice>(key));

        rc = catalog_->Update(&batch);
        if (!rc.ok()) {
            LOGERROR << "Failed to delete object for blob: '" << blobName << "' volume: '"
                << std::hex << volId_ << std::dec << "'";
            break;
        }
    }

    return rc;
}

Error DmPersistVolDB::deleteBlobMetaDesc(const std::string & blobName) {
    //IS_OP_ALLOWED();

    LOGDEBUG << "Deleting metadata for blob: '" << blobName << "' volume: '" << std::hex
             << volId_ << std::dec << "'";

    BlobMetadataKey const key {blobName};

    CatWriteBatch batch;
    TIMESTAMP_OP(batch);
    batch.Delete(static_cast<leveldb::Slice>(key));
    return catalog_->Update(&batch);
}

Error DmPersistVolDB::getInMemorySnapshot(Catalog::MemSnap& snap) {
    catalog_->GetSnapshot(snap);
    snapshotCount.fetch_add(1, std::memory_order_relaxed);
    return ERR_OK;
}

Error DmPersistVolDB::freeInMemorySnapshot(Catalog::MemSnap& snap)  {
    catalog_->ReleaseSnapshot(snap);
    snapshotCount.fetch_sub(1, std::memory_order_relaxed);
    return ERR_OK;
}

void DmPersistVolDB::forEachObject(std::function<void(const ObjectID&)> func) {
    auto dbIt = catalog_->NewIterator();
    Error err;
    ObjectID objId;
    fds_assert(dbIt);
    for (dbIt->Seek(BlobObjectKey(std::string()));
         dbIt->Valid()
         && *reinterpret_cast<CatalogKeyType const*>(dbIt->key().data())
         == CatalogKeyType::BLOB_OBJECTS;
         dbIt->Next())
    {
        leveldb::Slice dbKey = dbIt->key();
        BlobObjectKey const objectKey { dbKey };

        objId.SetId(dbIt->value().ToString());
        // LOGDEBUG << "blobid:" << blobKey->blobId << " idx:" << blobKey->objIndex << " " << objId;
        func(objId);
    }
    fds_assert(dbIt->status().ok());  // check for any errors during the scan
}

void DmPersistVolDB::getObjectIds(const uint32_t &maxObjs,
                                  const Catalog::MemSnap &snap,
                                  std::unique_ptr<Catalog::catalog_iterator_t>& dbItr,
                                  std::list<ObjectID> &objects) {
    objects.clear();

    if (dbItr == nullptr) {
        dbItr = catalog_->NewIterator(snap);
        dbItr->SeekToFirst();
    }

    for (; dbItr->Valid() && objects.size() < maxObjs; dbItr->Next()) {
        if (*reinterpret_cast<CatalogKeyType const*>(dbItr->key().data()) == CatalogKeyType::BLOB_OBJECTS) {
            objects.push_back(ObjectID(dbItr->value().ToString()));
        }
    }
}

int32_t DmPersistVolDB::getVersion()
{
    int32_t version;
    std::ifstream in(getVersionFile_());
    if (!in.is_open()) {
        return VolumeGroupConstants::VERSION_INVALID;
    }
    in >> version;
    in.close();
    return version;
}

void DmPersistVolDB::setVersion(int32_t version)
{
    std::ofstream out(getVersionFile_());
    out << version;
    out.close();
}

std::string DmPersistVolDB::getVersionFile_()
{
    const FdsRootDir* root = MODULEPROVIDER()->proc_fdsroot();
    return util::strformat("%s/%ld/version",
                           root->dir_sys_repo_dm().c_str(), srcVolId_.get());
}
}  // namespace fds
