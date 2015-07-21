/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#define _LARGEFILE64_SOURCE  // should be before all includes

#include <errno.h>

#include <string>
#include <vector>

#include <dm-vol-cat/DmPersistVolFile.h>

namespace fds {

const fds_uint32_t DmPersistVolFile::MMAP_CACHE_SIZE = 100;

DmPersistVolFile::DmPersistVolFile(fds_volid_t volId, fds_uint32_t objSize,
                   fds_bool_t snapshot, fds_bool_t readOnly,
                   fds_volid_t srcVolId /* = invalid_vol_id */)
        : DmPersistVolCat(volId, objSize, snapshot, readOnly, fpi::FDSP_VOL_BLKDEV_TYPE,
        srcVolId), metaFd_(-1), objFd_(-1), configHelper_(g_fdsprocess->get_conf_helper()) {
    mmapCache_.reset(new SharedKvCache<fds_uint64_t, DmOIDArrayMmap>(
            std::string("DmOIDArrayMmap") + std::to_string(volId), MMAP_CACHE_SIZE));

    const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
    dirname_ = snapshot_ ? root->dir_user_repo_dm() : root->dir_sys_repo_dm();
    if (!snapshot_ && srcVolId_ == invalid_vol_id) {
        // volume
        dirname_ += getVolIdStr();
        dirname_ += "/" + getVolIdStr() + "_vcat.ldb";
    } else if (srcVolId_ > invalid_vol_id && !snapshot_) {
        // clone
        dirname_ += getVolIdStr();
        dirname_ += "/" + getVolIdStr() + "_vcat.ldb";
    } else {
        // snapshot
        dirname_ += std::to_string(srcVolId_) + "/snapshot";
        dirname_ += "/" + getVolIdStr() + "_vcat.ldb";
    }

    objFilename_ = dirname_ + "/objects.oid";
    metaFilename_ = dirname_ + "/objects.meta";
}

DmPersistVolFile::~DmPersistVolFile() {
    mmapCache_->clear();
    close(metaFd_);
    close(objFd_);

    if (deleted_) {
        std::string rmCmd = "rm -rf " + dirname_;
        int rc = system(rmCmd.c_str());
        LOGNOTIFY << "Removed catalog directory, retcode = '" << rc << "'";
    }
}

Error DmPersistVolFile::activate() {
    LOGNOTIFY << "Activating '" << dirname_ << "'";
    FdsRootDir::fds_mkdir(dirname_.c_str());

    objFd_ = open(objFilename_.c_str(), O_RDWR | O_ASYNC | O_CREAT | O_LARGEFILE,
            S_IRUSR | S_IWUSR | S_IRGRP);
    if (objFd_ < 0) {
        LOGERROR << "Failed to open/ create file: '" << objFilename_ << "' errno: '"
                << errno << "'";
        return ERR_DISK_WRITE_FAILED;
    }

    metaFd_ = open(metaFilename_.c_str(), O_RDWR | O_ASYNC | O_CREAT | O_LARGEFILE,
            S_IRUSR | S_IWUSR | S_IRGRP);
    if (metaFd_ < 0) {
        LOGERROR << "Failed to open/ create file: '" << metaFilename_ << "' errno: '"
                << errno << "'";
        return ERR_DISK_WRITE_FAILED;
    }

    s_.reset(serialize::getFileSerializer(metaFd_));
    fds_verify(s_);

    d_.reset(serialize::getFileDeserializer(metaFd_));
    fds_verify(d_);

    return ERR_OK;
}

Error DmPersistVolFile::copyVolDir(const std::string & destName) {
    fds_assert(!destName.empty());
    FdsRootDir::fds_mkdir(destName.c_str());

    std::ostringstream oss;
    oss << "cp -rf " << dirname_ << "/* " << destName << "/";

    // TODO(umesh): use staging are while copying
    // XXX: this is not thread-safe
    int rc = system(oss.str().c_str());
    if (0 != rc) {
        LOGERROR << "Failed to copy directory '" << dirname_ << "' to '" << destName << "'";
        return ERR_DISK_WRITE_FAILED;
    }

    return ERR_OK;
}

Error DmPersistVolFile::getVolumeMetaDesc(VolumeMetaDesc & volDesc) {
    return ERR_OK;
}

Error DmPersistVolFile::getBlobMetaDesc(const std::string & blobName, BlobMetaDesc & blobMeta) {
    fds_scoped_lock sl(metaLock_);
    if (!blobMeta_) {
        blobMeta_.reset(new BlobMetaDesc());
        if (lseek(metaFd_, 0, SEEK_SET) < 0) {
            LOGERROR << "Failed to retrieve metadata for blob: '" << blobName << "' volume: '"
                    << std::hex << volId_ << std::dec << "'";
            return ERR_DISK_READ_FAILED;
        }

        struct stat64 st = {0};
        if (fstat64(metaFd_, &st) < 0 || 0 == st.st_size) {
            return ERR_CAT_ENTRY_NOT_FOUND;
        }

        if (!blobMeta_->read(d_.get())) {
            LOGWARN << "Deserialization failed of metadata for blob: '" << blobName <<
                    "' volume: '" << std::hex << volId_ << std::dec << "'";
            return ERR_CAT_ENTRY_NOT_FOUND;
        }
    }
    blobMeta = *(blobMeta_.get());
    return ERR_OK;
}

Error DmPersistVolFile::getAllBlobMetaDesc(std::vector<BlobMetaDesc> & blobMetaList) {
    BlobMetaDesc blobMeta;
    Error rc = getBlobMetaDesc("", blobMeta);
    if (rc.ok()) {
        blobMetaList.push_back(blobMeta);
    }
    return rc;
}

Error DmPersistVolFile::getObject(const std::string & blobName, fds_uint64_t offset,
        ObjectID & obj) {
    fds_verify(0 == offset % objSize_);

    fds_uint64_t objIndex = offset / objSize_;
    fds_uint64_t mmapId = objIndex / DmOIDArrayMmap::NUM_OBJS_PER_FRAGMENT;

    boost::shared_ptr<DmOIDArrayMmap> oidArrayMmap;
    Error rc = getOIDArrayMmap(mmapId, oidArrayMmap);
    if (rc.ok()) {
        rc = oidArrayMmap->getObject(objIndex, obj);
    }

    if (!rc.ok()) {
        LOGNOTIFY << "Failed to get oid for offset: '" << std::hex << offset << std::dec
                << "' blob: '" << blobName << "' volume: '" << std::hex << volId_ <<
                std::dec << "'";
    } else if (NullObjectID == obj) {
        return ERR_CAT_ENTRY_NOT_FOUND;
    }
    return rc;
}

Error DmPersistVolFile::getObject(const std::string & blobName, fds_uint64_t startOffset,
        fds_uint64_t endOffset, BlobObjList & objList) {
    fds_assert(startOffset <= endOffset);
    fds_verify(0 == startOffset % objSize_);
    fds_verify(0 == endOffset % objSize_);

    Error rc = ERR_OK;
    boost::shared_ptr<DmOIDArrayMmap> oidArrayMmap;
    for (fds_uint64_t offset = startOffset; offset <= endOffset; offset += objSize_) {
        fds_uint64_t objIndex = offset / objSize_;
        fds_uint64_t mmapId = objIndex / DmOIDArrayMmap::NUM_OBJS_PER_FRAGMENT;

        if (!oidArrayMmap || oidArrayMmap->getID() != mmapId) {
            rc = getOIDArrayMmap(mmapId, oidArrayMmap);
            if (!rc.ok()) {
                LOGNOTIFY << "Failed to get fragment for offset: '" << std::hex << offset <<
                        std::dec << "' blob: '" << blobName << "' volume: '" << std::hex <<
                        volId_ << std::dec << "'";
                continue;
            }
        }

        ObjectID obj;
        rc = oidArrayMmap->getObject(objIndex, obj);
        if (!rc.ok()) {
            LOGNOTIFY << "Failed to get oid for offset: '" << std::hex << offset << std::dec
                    << "' blob: '" << blobName << "' volume: '" << std::hex << volId_ <<
                    std::dec << "'";
            continue;
        }
        if (NullObjectID == obj) {
            continue;
        }

        BlobObjInfo blobInfo;
        blobInfo.size = getObjSize();
        blobInfo.oid = obj;
        objList[objIndex * objSize_] = blobInfo;
    }

    return ERR_OK;
}

Error DmPersistVolFile::getObject(const std::string & blobName, fds_uint64_t startOffset,
        fds_uint64_t endOffset, fpi::FDSP_BlobObjectList& objList) {
    fds_assert(startOffset <= endOffset);
    fds_verify(0 == startOffset % objSize_);
    fds_verify(0 == endOffset % objSize_);

    Error rc = ERR_OK;
    boost::shared_ptr<DmOIDArrayMmap> oidArrayMmap;
    for (fds_uint64_t offset = startOffset; offset <= endOffset; offset += objSize_) {
        fds_uint64_t objIndex = offset / objSize_;
        fds_uint64_t mmapId = objIndex / DmOIDArrayMmap::NUM_OBJS_PER_FRAGMENT;

        if (!oidArrayMmap || oidArrayMmap->getID() != mmapId) {
            rc = getOIDArrayMmap(mmapId, oidArrayMmap);
            if (!rc.ok()) {
                LOGNOTIFY << "Failed to get fragment for offset: '" << std::hex << offset <<
                        std::dec << "' blob: '" << blobName << "' volume: '" << std::hex <<
                        volId_ << std::dec << "'";
                continue;
            }
        }

        ObjectID obj;
        rc = oidArrayMmap->getObject(objIndex, obj);
        if (!rc.ok()) {
            LOGNOTIFY << "Failed to get oid for offset: '" << std::hex << offset << std::dec
                    << "' blob: '" << blobName << "' volume: '" << std::hex << volId_ <<
                    std::dec << "'";
            continue;
        }
        if (NullObjectID == obj) {
            continue;
        }

        fpi::FDSP_BlobObjectInfo blobInfo;
        blobInfo.offset = objIndex * objSize_;
        blobInfo.size = objSize_;
        std::string objStr(reinterpret_cast<const char *>(obj.GetId()), obj.GetLen());
        blobInfo.data_obj_id.digest = objStr;

        objList.push_back(blobInfo);
    }

    return ERR_OK;
}

Error DmPersistVolFile::putVolumeMetaDesc(const VolumeMetaDesc & volDesc) {
    return ERR_OK;
}

Error DmPersistVolFile::putBlobMetaDesc(const std::string & blobName,
        const BlobMetaDesc & blobMeta) {
    IS_OP_ALLOWED();

    fds_scoped_lock sl(metaLock_);
    *(blobMeta_.get()) = blobMeta;
    if (lseek(metaFd_, 0, SEEK_SET) < 0) {
        LOGERROR << "Failed to write metadata for blob: '" << blobName << "' volume: '"
                << std::hex << volId_ << std::dec << "'";
        return ERR_DISK_WRITE_FAILED;
    }

    if (!blobMeta_->write(s_.get())) {
        LOGERROR << "Serialization failed of metadata for blob: '" << blobName <<
                "' volume: '" << std::hex << volId_ << std::dec << "'";
        return ERR_DISK_WRITE_FAILED;
    }

    return ERR_OK;
}

Error DmPersistVolFile::putObject(const std::string & blobName, fds_uint64_t offset,
        const ObjectID & obj) {
    IS_OP_ALLOWED();

    fds_verify(0 == offset % objSize_);

    fds_uint64_t objIndex = offset / objSize_;
    fds_uint64_t mmapId = objIndex / DmOIDArrayMmap::NUM_OBJS_PER_FRAGMENT;

    boost::shared_ptr<DmOIDArrayMmap> oidArrayMmap;
    Error rc = getOIDArrayMmap(mmapId, oidArrayMmap, false);
    if (rc.ok()) {
        rc = oidArrayMmap->putObject(objIndex, obj);
    }

    if (!rc.ok()) {
        LOGNOTIFY << "Failed to write oid for offset: '" << std::hex << offset << std::dec
                << "' blob: '" << blobName << "' volume: '" << std::hex << volId_ <<
                std::dec << "'";
    }
    return rc;
}

Error DmPersistVolFile::putObject(const std::string & blobName, const BlobObjList & objs) {
    IS_OP_ALLOWED();

    Error rc = ERR_OK;
    if (objs.empty()) {
        return rc;
    }

    boost::shared_ptr<DmOIDArrayMmap> oidArrayMmap;

    for (auto & it : objs) {
        fds_verify(0 == it.first % objSize_);

        fds_uint64_t objIndex = it.first / objSize_;
        fds_uint64_t mmapId = objIndex / DmOIDArrayMmap::NUM_OBJS_PER_FRAGMENT;

        if (!oidArrayMmap || oidArrayMmap->getID() != mmapId) {
            rc = getOIDArrayMmap(mmapId, oidArrayMmap, false);
            if (!rc.ok()) {
                LOGNOTIFY << "Failed to get fragment for offset: '" << std::hex << it.first <<
                        std::dec << "' blob: '" << blobName << "' volume: '" << std::hex <<
                        volId_ << std::dec << "'";
                break;
            }
        }

        rc = oidArrayMmap->putObject(objIndex, it.second.oid);
        if (!rc.ok()) {
            LOGERROR << "Failed to write oid for offset: '" << std::hex << it.first << std::dec
                    << "' blob: '" << blobName << "' volume: '" << std::hex << volId_ <<
                    std::dec << "'";
            break;
        }
    }

    return rc;
}

Error DmPersistVolFile::putBatch(const std::string & blobName, const BlobMetaDesc & blobMeta,
            const BlobObjList & puts, const std::vector<fds_uint64_t> & deletes) {
    IS_OP_ALLOWED();

    Error rc = putBlobMetaDesc(blobName, blobMeta);
    if (rc.ok()) {
        rc = putObject(blobName, puts);
        if (rc.ok()) {
            for (auto & it : deletes) {
                rc = deleteObject(blobName, it);
                if (!rc.ok()) {
                    break;
                }
            }
        }
    }

    return rc;
}

Error DmPersistVolFile::putBatch(const std::string & blobName, const BlobMetaDesc & blobMeta,
            CatWriteBatch & wb) {
    // TODO(umesh): implement this
    return ERR_OK;
}

Error DmPersistVolFile::deleteObject(const std::string & blobName, fds_uint64_t offset) {
    return putObject(blobName, offset, NullObjectID);
}

Error DmPersistVolFile::deleteObject(const std::string & blobName, fds_uint64_t startOffset,
        fds_uint64_t endOffset) {
    BlobObjList objList;
    for (fds_uint64_t offset = startOffset; offset <= endOffset; offset += objSize_) {
        BlobObjInfo blobInfo;
        blobInfo.size = getObjSize();
        blobInfo.oid = NullObjectID;
        objList[offset] = blobInfo;
    }

    return putObject(blobName, objList);
}

Error DmPersistVolFile::deleteBlobMetaDesc(const std::string & blobName) {
    blobMeta_.reset();
    if (ftruncate64(metaFd_, 0) < 0) {
        return ERR_DISK_WRITE_FAILED;
    }
    return ERR_OK;
}

Error DmPersistVolFile::getOIDArrayMmap(fds_uint64_t id,
        boost::shared_ptr<DmOIDArrayMmap> & oidArrayMmap, fds_bool_t read /* = true */) {
    fds_scoped_lock sl(cacheLock_);
    Error rc = mmapCache_->get(id, oidArrayMmap);
    if (rc.ok()) {
        return rc;  // we found mmap in cache
    }

    // check if requested fragment is already in file
    struct stat64 st = {0};
    if (fstat64(objFd_, &st) < 0) {
        LOGNOTIFY << "Failed to get stat details for file: '" << objFilename_ << "'";
        return ERR_DISK_READ_FAILED;
    }

    off64_t idOffset= id * DmOIDArrayMmap::FRAGMENT_SIZE;
    if (idOffset >= st.st_size && read) {
        return ERR_CAT_ENTRY_NOT_FOUND;
    }

    oidArrayMmap.reset(new DmOIDArrayMmap(id, objFd_));
    mmapCache_->add(id, oidArrayMmap);

    return ERR_OK;
}

void DmPersistVolFile::forEachObject(std::function<void(const ObjectID&)>) {
    throw fds::Exception(ERR_NOT_IMPLEMENTED);
}
}  // namespace fds
