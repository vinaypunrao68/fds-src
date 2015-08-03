/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <utility>
#include <string>
#include <vector>
#include <map>

#include <fds_process.h>
#include <dm-tvc/CommitLog.h>
#include <dm-vol-cat/DmPersistVolCat.h>

namespace fds {

template Error DmCommitLog::updateTx(BlobTxId::const_ptr & txDesc,
                        boost::shared_ptr<const MetaDataList> & blobData);
template Error DmCommitLog::updateTx(BlobTxId::const_ptr & txDesc,
                        boost::shared_ptr<const BlobObjList> & blobData);
template Error DmCommitLog::updateTx(BlobTxId::const_ptr & txDesc,
                        const fpi::FDSP_BlobObjectList & blobData);

uint32_t CommitLogTx::write(serialize::Serializer * s) const {
    fds_assert(s);
    fds_assert(txDesc);
    fds_assert(blobObjList);
    fds_assert(metaDataList);

    GLOGTRACE << "Serializing transaction '" << *txDesc << "'";

    uint32_t bytes = s->writeI64(txDesc->getValue());
    bytes += s->writeString(name);
    bytes += s->writeI32(blobMode);

    bytes += s->writeI64(started);
    bytes += s->writeI64(committed);
    bytes += s->writeBool(blobDelete);
    bytes += s->writeBool(snapshot);

    bytes += blobObjList->write(s);
    bytes += metaDataList->write(s);

    bytes += s->writeI64(blobVersion);

    return bytes;
}

uint32_t CommitLogTx::read(serialize::Deserializer * d) {
    fds_assert(d);

    fds_uint64_t txId = 0;
    uint32_t bytes = d->readI64(txId);
    txDesc.reset(new BlobTxId(txId));

    GLOGTRACE << "Deserializing transaction '" << *txDesc << "'";

    bytes += d->readString(name);
    bytes += d->readI32(blobMode);

    bytes += d->readI64(started);
    bytes += d->readI64(committed);
    bytes += d->readBool(blobDelete);
    bytes += d->readBool(snapshot);

    bytes += blobObjList->read(d);
    bytes += metaDataList->read(d);

    bytes += d->readI64(blobVersion);

    return bytes;
}

DmCommitLog::DmCommitLog(const std::string &modName, const fds_volid_t volId,
        const fds_uint32_t objSize) : Module(modName.c_str()), volId_(volId), objSize_(objSize),
        started_(false) {
    std::ostringstream oss;
    const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
    oss << root->dir_sys_repo_dm() << volId_;
    FdsRootDir::fds_mkdir(oss.str().c_str());

    oss.str("");
    oss << root->dir_user_repo_dm() << volId_;
    FdsRootDir::fds_mkdir(oss.str().c_str());
}

DmCommitLog::~DmCommitLog() {}

/**
 * Module initialization
 */
int
DmCommitLog::mod_init(SysParams const *const p) {
    Module::mod_init(p);
    return 0;
}

/**
 * Module startup
 */
void
DmCommitLog::mod_startup() {
    Module::mod_startup();
    started_ = true;
}

/**
 * Module shutdown
 */
void
DmCommitLog::mod_shutdown() {
    Module::mod_shutdown();
    started_ = false;
}

/*
 * operations
 */
// start transaction
Error DmCommitLog::startTx(BlobTxId::const_ptr & txDesc, const std::string & blobName,
                           fds_int32_t blobMode,
                           fds_uint64_t dmtVersion) {
    fds_assert(txDesc);
    fds_verify(started_);

    const BlobTxId & txId = *txDesc;

    GLOGDEBUG << "Starting blob transaction (" << txId << ") for (" << blobName << ")";

    auto ptx = boost::make_shared<CommitLogTx>();

    {
        std::lock_guard<std::mutex> guard(lockTxMap_);
        TxMap::const_iterator logIt = txMap_.find(txId);
        if (txMap_.end() != logIt) {
            GLOGWARN << "Blob transaction already exists";
            return ERR_DM_TX_EXISTS;
        }
        txMap_[txId] = ptx;
        ++dmtVerMap_[dmtVersion];

        ptx->txDesc = txDesc;
        ptx->name = blobName;
        ptx->blobMode = blobMode;
        ptx->started = util::getTimeStampNanos();
        ptx->nameId = DmPersistVolCat::getBlobIdFromName(blobName);
        ptx->dmtVersion = dmtVersion;
    }

    return ERR_OK;
}

// update blob data (T can be BlobObjList or MetaDataList)
template<typename T>
Error DmCommitLog::updateTx(BlobTxId::const_ptr & txDesc, boost::shared_ptr<const T> & blobData) {
    fds_assert(txDesc);
    fds_assert(blobData);
    fds_verify(started_);

    const BlobTxId & txId = *txDesc;

    GLOGDEBUG << "Update blob for transaction (" << txId << ")";

    std::lock_guard<std::mutex> guard(lockTxMap_);

    Error rc = validateSubsequentTx(txId);
    if (!rc.ok()) {
        return rc;
    }

    upsertBlobData(*txMap_[txId], blobData);

    return rc;
}

// update blob data (T can be fpi::FDSP_BlobObjectList or fpi::FDSP_MetaDataList
template<typename T>
Error DmCommitLog::updateTx(BlobTxId::const_ptr & txDesc, const T & blobData) {
    fds_assert(txDesc);
    fds_verify(started_);

    const BlobTxId & txId = *txDesc;

    GLOGDEBUG << "Update blob for transaction (" << txId << ")";

    std::lock_guard<std::mutex> guard(lockTxMap_);

    Error rc = validateSubsequentTx(txId);
    if (!rc.ok()) {
        return rc;
    }

    upsertBlobData(*txMap_[txId], blobData);

    return rc;
}

void DmCommitLog::upsertBlobData(CommitLogTx & tx, const fpi::FDSP_BlobObjectList & data) {
    fds_uint64_t newSize = 0;
    BlobObjKey objKey(tx.nameId, 0);
    for (const auto & objInfo : data) {
        fds_verify(0 == objInfo.offset % objSize_);
        fds_verify(0 < objInfo.size);
        fds_verify((tx.blobMode | blob::TRUNCATE) || (objSize_ == objInfo.size));
        newSize = objInfo.offset + objInfo.size;
        if (tx.blobSize < newSize) {
            tx.blobSize = newSize;
        }

        objKey.objIndex = objInfo.offset / objSize_;
        const Record keyRec(reinterpret_cast<const char *>(&objKey), sizeof(BlobObjKey));
        tx.wb.Put(keyRec, objInfo.data_obj_id.digest);
    }
}

// delete blob
Error DmCommitLog::deleteBlob(BlobTxId::const_ptr & txDesc, const blob_version_t blobVersion) {
    fds_assert(txDesc);
    fds_verify(started_);

    const BlobTxId & txId = *txDesc;

    GLOGDEBUG << "Delete blob in transaction (" << txId << ")";

    std::lock_guard<std::mutex> guard(lockTxMap_);

    Error rc = validateSubsequentTx(txId);
    if (!rc.ok()) {
        return rc;
    }

    CommitLogTx::ptr & ptx = txMap_[txId];
    ptx->blobDelete = true;
    ptx->blobVersion = blobVersion;

    return rc;
}

// commit transaction
CommitLogTx::ptr DmCommitLog::commitTx(BlobTxId::const_ptr & txDesc, Error & status) {
    fds_assert(txDesc);
    fds_verify(started_);

    const BlobTxId & txId = *txDesc;

    GLOGDEBUG << "Committing blob transaction " << txId;

    std::lock_guard<std::mutex> guard(lockTxMap_);

    status = validateSubsequentTx(txId);
    if (!status.ok()) {
        return 0;
    }

    CommitLogTx::ptr ptx = txMap_[txId];
    ptx->committed = util::getTimeStampNanos();
    --dmtVerMap_[ptx->dmtVersion];
    txMap_.erase(txId);

    return ptx;
}

// rollback transaction
Error DmCommitLog::rollbackTx(BlobTxId::const_ptr & txDesc) {
    fds_assert(txDesc);
    fds_verify(started_);

    const BlobTxId & txId = *txDesc;

    GLOGDEBUG << "Rollback blob transaction " << txId;

    std::lock_guard<std::mutex> guard(lockTxMap_);

    Error rc = validateSubsequentTx(txId);
    if (!rc.ok()) {
        return rc;
    }

    CommitLogTx::ptr ptx = txMap_[txId];
    --dmtVerMap_[ptx->dmtVersion];
    txMap_.erase(txId);

    return rc;
}

// snapshot
Error DmCommitLog::snapshot(BlobTxId::const_ptr & txDesc, const std::string & name) {
    const BlobTxId & txId = *txDesc;

    Error rc = startTx(txDesc, name, 0, 0);
    if (!rc.ok()) {
        GLOGWARN << "Failed to start transaction '" << txId << "' for snapshot '" << name << "'";
        return rc;
    }

    rc = snapshotInsert(txDesc);
    if (!rc.ok()) {
        GLOGWARN << "Failed to insert snapshot for transaction '" << txId << "'";
        return rc;
    }

    CommitLogTx::const_ptr tx = commitTx(txDesc, rc);
    if (!rc.ok()) {
        GLOGWARN << "Failed to commit snapshot transaction '" << txId << "'";
        return rc;
    }

    return rc;
}

// get transaction
CommitLogTx::const_ptr DmCommitLog::getTx(BlobTxId::const_ptr & txDesc) {
    fds_assert(txDesc);
    fds_verify(started_);

    const BlobTxId & txId = *txDesc;

    GLOGDEBUG << "Get blob transaction " << txId;

    std::lock_guard<std::mutex> guard(lockTxMap_);

    TxMap::const_iterator iter = txMap_.find(txId);
    if (txMap_.end() != iter) {
        return iter->second;
    }

    return 0;
}

/*
 * when calling this, you must hold lockTxMap_
 */
Error DmCommitLog::validateSubsequentTx(const BlobTxId & txId) {
    TxMap::iterator iter = txMap_.find(txId);
    if (txMap_.end() == iter) {
        GLOGERROR << "Blob transaction not started";
        return ERR_DM_TX_NOT_STARTED;
    }
    fds_assert(txId == *(iter->second->txDesc));

    if (iter->second->committed) {
        GLOGERROR << "Blob transaction already committed";
        return ERR_DM_TX_COMMITTED;
    }

    return ERR_OK;
}

fds_bool_t DmCommitLog::isPendingTx(const fds_uint64_t tsNano /* = util::getTimeStampNanos() */) {
    fds_bool_t ret = false;
    std::lock_guard<std::mutex> guard(lockTxMap_);
    for (auto it : txMap_) {
        if (it.second->started && !it.second->committed) {
            if (it.second->started <= tsNano) {
                ret = true;
            }
        }
    }

    return ret;
}

Error DmCommitLog::snapshotInsert(BlobTxId::const_ptr & txDesc) {
    fds_assert(txDesc);
    fds_verify(started_);

    const BlobTxId & txId = *txDesc;

    GLOGDEBUG << "Snapshot transaction " << txId;

    std::lock_guard<std::mutex> guard(lockTxMap_);

    Error rc = validateSubsequentTx(txId);
    if (!rc.ok()) {
        return rc;
    }

    txMap_[txId]->snapshot = true;

    return rc;
}

bool DmCommitLog::checkOutstandingTx(fds_uint64_t dmtVersion) {
    std::lock_guard<std::mutex> guard(lockTxMap_);

    auto it = dmtVerMap_.find(dmtVersion);

    if (it == dmtVerMap_.end()) {
        return false;
    }

    if (0 == it->second) {
        dmtVerMap_.erase(dmtVersion);
        return false;
    }

    return true;
}

}  /* namespace fds */
