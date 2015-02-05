/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMPERSISTVOLCAT_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMPERSISTVOLCAT_H_

#include <map>
#include <string>
#include <vector>

#include <fds_uuid.h>

#include <lib/Catalog.h>
#include <DmBlobTypes.h>
#include <dm-tvc/CommitLog.h>

#define IS_OP_ALLOWED() \
    if (isSnapshot() || isReadOnly()) { \
        LOGWARN << "Volume is either snapshot or read only. Updates not allowed"; \
        return ERR_DM_OP_NOT_ALLOWED; \
    }

namespace fds {

extern const fds_uint64_t INVALID_BLOB_ID;
extern const fds_uint32_t BLOB_META_INDEX;

struct BlobObjKey {
    fds_uint64_t blobId;
    fds_uint32_t objIndex;

    BlobObjKey() : blobId(INVALID_BLOB_ID), objIndex(BLOB_META_INDEX) {}

    explicit BlobObjKey(fds_uint64_t blobId_, fds_uint32_t objIndex_ = BLOB_META_INDEX)
            : blobId(blobId_), objIndex(objIndex_) {}
};

extern const BlobObjKey OP_TIMESTAMP_KEY;
extern const Record OP_TIMESTAMP_REC;

class DmPersistVolDir {
  public:
    // types
    typedef boost::shared_ptr<DmPersistVolDir> ptr;
    typedef boost::shared_ptr<const DmPersistVolDir> const_ptr;

    // ctor and dtor
    DmPersistVolDir(fds_volid_t volId, fds_uint32_t objSize,
                    fds_bool_t snapshot,
                    fds_bool_t readOnly,
                    fpi::FDSP_VolType volType = fpi::FDSP_VOL_S3_TYPE,
                    fds_volid_t srcVolId = invalid_vol_id) : volId_(volId), objSize_(objSize),
            volType_(volType), srcVolId_(srcVolId), snapshot_(snapshot), readOnly_(readOnly),
            initialized_(false), deleted_(false) {
        fds_verify(objSize > 0);
        if (invalid_vol_id == srcVolId_) {
            srcVolId_ = volId;
        }
        if (snapshot_) {
            fds_verify(readOnly_);
        }
    }

    virtual ~DmPersistVolDir() {}

    // methods
    inline fds_bool_t isInitialized() const {
        return initialized_;
    }

    inline fds_volid_t getVolId() const {
        return volId_;
    }

    inline std::string getVolIdStr() const {
        return std::to_string(volId_);
    }

    inline fpi::FDSP_VolType getVolType() const {
        return volType_;
    }

    inline fds_uint32_t getObjSize() const {
        return objSize_;
    }

    inline fds_volid_t getSrcVolId() const {
        return srcVolId_;
    }

    inline fds_bool_t isSnapshot() const {
        return snapshot_;
    }

    inline fds_bool_t isReadOnly() const {
        return readOnly_;
    }

    // creation and deletion
    virtual Error activate() = 0;

    virtual Error copyVolDir(const std::string & destName) = 0;

    virtual Error markDeleted() {
        LOGNOTIFY << "Catalog for volume '" << volId_ << "' marked deleted";
        deleted_ = true;
        return ERR_OK;
    }

    virtual fds_bool_t isMarkedDeleted() const {
        return deleted_;
    }

    // gets
    virtual Error getBlobMetaDesc(const std::string & blobName, BlobMetaDesc & blobMeta) = 0;

    virtual Error getAllBlobMetaDesc(std::vector<BlobMetaDesc> & blobMetaList) = 0;

    virtual Error getObject(const std::string & blobName, fds_uint64_t offset,
            ObjectID & obj) = 0;

    virtual Error getObject(const std::string & blobName, fds_uint64_t startOffset,
            fds_uint64_t endOffset, fpi::FDSP_BlobObjectList& objList) = 0;

    virtual Error getObject(const std::string & blobName, fds_uint64_t startOffset,
            fds_uint64_t endOffset, BlobObjList & objList) = 0;

    // puts
    virtual Error putBlobMetaDesc(const std::string & blobName,
            const BlobMetaDesc & blobMeta) = 0;

    virtual Error putObject(const std::string & blobName, fds_uint64_t offset,
            const ObjectID & obj) = 0;

    virtual Error putObject(const std::string & blobName, const BlobObjList & objs) = 0;

    virtual Error putBatch(const std::string & blobName, const BlobMetaDesc & blobMeta,
            const BlobObjList & puts, const std::vector<fds_uint64_t> & deletes) = 0;

    virtual Error putBatch(const std::string & blobName, const BlobMetaDesc & blobMeta,
            CatWriteBatch & wb) = 0;

    // deletes
    virtual Error deleteObject(const std::string & blobName, fds_uint64_t offset) = 0;

    virtual Error deleteObject(const std::string & blobName, fds_uint64_t startOffset,
            fds_uint64_t endOffset) = 0;

    virtual Error deleteBlobMetaDesc(const std::string & blobName) = 0;

    // sync
    virtual Error syncCatalog(const NodeUuid & dmUuid);

    friend DmCommitLog;

  protected:
    // methods
    static inline fds_uint64_t getBlobIdFromName(const std::string & blobName) {
        return fds_get_uuid64(blobName);
    }

    // vars
    fds_volid_t volId_;
    fds_uint32_t objSize_;
    fpi::FDSP_VolType volType_;

    fds_volid_t srcVolId_;
    fds_bool_t snapshot_;
    fds_bool_t readOnly_;

    fds_bool_t initialized_;
    fds_bool_t deleted_;

    // TODO(Andrew): This is totally insecure. If we're gonna do this,
    // we need the other service's public key. Or we move the data over
    // our own secure channel.
    /// Username for rsync migration
    std::string rsyncUser;
    /// Password for rsync migration
    std::string rsyncPasswd;
};
}  // namespace fds
#endif  // SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMPERSISTVOLCAT_H_
