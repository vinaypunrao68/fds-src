/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMPERSISTVOLCAT_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMPERSISTVOLCAT_H_

// Standard includes.
#include <map>
#include <string>
#include <vector>

// Internal includes.
#include "catalogKeys/JournalTimestampKey.h"
#include "lib/Catalog.h"
#include "DmBlobTypes.h"
#include "fds_uuid.h"
#include <fds_module_provider.h>

#define IS_OP_ALLOWED() \
    if (isSnapshot() || isReadOnly()) { \
        LOGWARN << "Volume is either snapshot or read only. Updates not allowed"; \
        return ERR_DM_OP_NOT_ALLOWED; \
    }

namespace fds {

extern const fds_uint64_t INVALID_BLOB_ID;
extern const fds_uint32_t BLOB_META_INDEX;
extern const fds_uint64_t VOL_META_ID;

extern const JournalTimestampKey OP_TIMESTAMP_KEY;
extern const leveldb::Slice OP_TIMESTAMP_REC;

/**
 * A cache of volume stats to avoid calculation by Data Object scan.
 * Size is in bytes.
 * Size and Data Object counts are "logical" (no deduplication applied).
 */
struct DmVolumeSummary {
    bool initialized{false};
    fds_uint64_t size{0};
    fds_uint64_t blobCount{0};
    fds_uint64_t objectCount{0};
};

class DmPersistVolCat : public HasModuleProvider {
  public:
    // types
    typedef boost::shared_ptr<DmPersistVolCat> ptr;
    typedef boost::shared_ptr<const DmPersistVolCat> const_ptr;

    // ctor and dtor
    DmPersistVolCat(CommonModuleProviderIf *modProvider,
                    fds_volid_t volId, fds_uint32_t objSize,
                    fds_bool_t snapshot,
                    fds_bool_t readOnly,
                    fds_bool_t clone,
                    fpi::FDSP_VolType volType = fpi::FDSP_VOL_S3_TYPE,
                    fds_volid_t srcVolId = invalid_vol_id)
        : HasModuleProvider(modProvider),
        volId_(volId), objSize_(objSize),
        volType_(volType), srcVolId_(srcVolId), snapshot_(snapshot), readOnly_(readOnly),
        clone_(clone), initialized_(false), deleted_(false), activated_(false)
    {
        fds_verify(objSize > 0);
        if (invalid_vol_id == srcVolId_) {
            srcVolId_ = volId;
        }
        if (snapshot_) {
            fds_verify(readOnly_);
        }
    }

    virtual ~DmPersistVolCat() {}

    // methods
    inline fds_bool_t isInitialized() const {
        return initialized_;
    }

    inline fds_volid_t getVolId() const {
        return volId_;
    }

    inline std::string getVolIdStr() const {
        return std::to_string(volId_.get());
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

    inline fds_bool_t isClone() const {
        return clone_;
    }

    inline fds_bool_t isActivated() const {
        return activated_;
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
    virtual Error getVolumeMetaDesc(VolumeMetaDesc & blobMeta) = 0;

    virtual Error getBlobMetaDesc(const std::string & blobName, BlobMetaDesc & blobMeta,
                                  const Catalog::MemSnap snap = NULL) = 0;

    virtual Error getAllBlobMetaDesc(std::vector<BlobMetaDesc> & blobMetaList) = 0;

    virtual Error getBlobMetaDescForPrefix(std::string const& prefix,
                                           std::string const& delimiter,
                                           std::vector<BlobMetaDesc>& blobMetaList,
                                           std::vector<std::string>& skippedPrefixes) = 0;

    virtual Error getObject(const std::string & blobName, fds_uint64_t offset,
            ObjectID & obj) = 0;

    virtual Error getObject(const std::string & blobName,
                            fds_uint64_t startOffset,
                            fds_uint64_t endOffset,
                            fpi::FDSP_BlobObjectList& objList,
                            const Catalog::MemSnap snap = NULL) = 0;

    virtual Error getObject(const std::string & blobName, fds_uint64_t startOffset,
            fds_uint64_t endOffset, BlobObjList & objList) = 0;

    virtual Error getLatestSequenceId(blob_version_t & max) = 0;

    virtual Error getAllBlobsWithSequenceId(std::map<std::string, int64_t>& blobsWithSeqId,
                                            const Catalog::MemSnap snap) = 0;

    virtual Error getAllBlobsWithSequenceId(std::map<std::string, int64_t>& blobsWithSeqId,
                                            const Catalog::MemSnap snap,
                                            const fds_bool_t &abortFlag) = 0;

    virtual Error getInMemorySnapshot(Catalog::MemSnap &snap) = 0;

    virtual void getObjectIds(const uint32_t &maxObjs,
                              const Catalog::MemSnap &snap,
                              std::unique_ptr<Catalog::catalog_iterator_t>& dbItr,
                              std::list<ObjectID> &objects) = 0;


    // puts
    virtual Error putVolumeMetaDesc(const VolumeMetaDesc & volDesc) = 0;

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

    virtual Error freeInMemorySnapshot(Catalog::MemSnap& snap) = 0;

    virtual uint64_t getNumInMemorySnapshots() = 0;

    /**
     * Returns false if the volume summary cache has not been initialized.
     */
    virtual bool volSummaryInitialized() = 0;

    /**
     * Returns ERR_DUPLICATE if the initialization has already succeeded.
     */
    virtual Error initVolSummary(fds_uint64_t logicalSize,
                                 fds_uint64_t blobCount,
                                 fds_uint64_t logicalObjectCount) = 0;

    /**
     * Returns ERR_NOT_READY if the volume summary cache has not been initialized.
     */
    virtual Error applyStatDeltas(const fds_uint64_t bytesAdded,
                                  const fds_uint64_t bytesRemoved,
                                  const fds_uint64_t blobsAdded,
                                  const fds_uint64_t blobsRemoved,
                                  const fds_uint64_t objectsAdded,
                                  const fds_uint64_t objectsRemoved) = 0;

    virtual void resetVolSummary() = 0;

    /**
     * Returns ERR_NOT_READY if the volume summary cache has not been initialized.
     */
    virtual Error getVolSummary(fds_uint64_t* logicalSize,
                                fds_uint64_t* blobCount,
                                fds_uint64_t* logicalObjectCount) = 0;

    // sync
    virtual Error syncCatalog(const NodeUuid & dmUuid);

    virtual void forEachObject(std::function<void(const ObjectID&)>) = 0;

    virtual int32_t getVersion() = 0;
    virtual void setVersion(int32_t version) = 0;

  protected:
    // vars
    fds_volid_t volId_;
    fds_uint32_t objSize_;
    fpi::FDSP_VolType volType_;

    fds_volid_t srcVolId_;
    fds_bool_t snapshot_;
    fds_bool_t readOnly_;
    fds_bool_t clone_;

    fds_bool_t initialized_;
    fds_bool_t deleted_;
    fds_bool_t activated_;

    // TODO(Andrew): This is totally insecure. If we're gonna do this,
    // we need the other service's public key. Or we move the data over
    // our own secure channel.
    /// Username for rsync migration
    std::string rsyncUser;
    /// Password for rsync migration
    std::string rsyncPasswd;

    DmVolumeSummary volSummary_;
    fds_mutex lockVolSummary_;

};
}  // namespace fds
#endif  // SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMPERSISTVOLCAT_H_
