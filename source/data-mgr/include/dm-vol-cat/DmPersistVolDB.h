/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMPERSISTVOLDB_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMPERSISTVOLDB_H_

#include <map>
#include <string>
#include <vector>

#include <leveldb/comparator.h>

#include <lib/Catalog.h>

#include <dm-vol-cat/DmPersistVolDir.h>

namespace fds {

class BlobObjKeyComparator : public leveldb::Comparator {
  public:
    int Compare(const leveldb::Slice & lhs, const leveldb::Slice & rhs) const {
        const BlobObjKey * lhsKey = reinterpret_cast<const BlobObjKey *>(lhs.data());
        const BlobObjKey * rhsKey = reinterpret_cast<const BlobObjKey *>(rhs.data());

        if (lhsKey->blobId < rhsKey->blobId) {
            return -1;
        } else if (lhsKey->blobId > rhsKey->blobId) {
            return 1;
        } else if (lhsKey->objIndex < rhsKey->objIndex) {
            return -1;
        } else if (lhsKey->objIndex > rhsKey->objIndex) {
            return 1;
        }

        return 0;
    }

    const char * Name() const {
        return "BlobObjKeyComparator";
    }

    void FindShortestSeparator(std::string *, const leveldb::Slice &) const {}
    void FindShortSuccessor(std::string*) const {}
};

class DmPersistVolDB : public HasLogger, public DmPersistVolDir {
  public:
    // types
    typedef boost::shared_ptr<DmPersistVolDB> ptr;
    typedef boost::shared_ptr<const DmPersistVolDB> const_ptr;

    static const std::string CATALOG_WRITE_BUFFER_SIZE_STR;
    static const std::string CATALOG_CACHE_SIZE_STR;
    static const std::string CATALOG_MAX_LOG_FILES_STR;

    // ctor & dtor
    DmPersistVolDB(fds_volid_t volId, fds_uint32_t objSize,
                   fds_bool_t snapshot, fds_bool_t readOnly,
                   fds_volid_t srcVolId = invalid_vol_id)
            : DmPersistVolDir(volId, objSize, snapshot, readOnly, fpi::FDSP_VOL_S3_TYPE,
            srcVolId), catalog_(0), configHelper_(g_fdsprocess->get_conf_helper()) {
        const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
        timelineDir_ = root->dir_timeline_dm() + getVolIdStr() + "/";
    }
    virtual ~DmPersistVolDB();

    // methods
    virtual Error activate() override;

    virtual Error copyVolDir(const std::string & destName) override;

    // gets
    virtual Error getBlobMetaDesc(const std::string & blobName,
            BlobMetaDesc & blobMeta) override;

    virtual Error getAllBlobMetaDesc(std::vector<BlobMetaDesc> & blobMetaList) override;

    virtual Error getObject(const std::string & blobName, fds_uint64_t offset,
            ObjectID & obj) override;

    virtual Error getObject(const std::string & blobName, fds_uint64_t startOffset,
            fds_uint64_t endOffset, fpi::FDSP_BlobObjectList& objList) override;

    virtual Error getObject(const std::string & blobName, fds_uint64_t startOffset,
            fds_uint64_t endOffset, BlobObjList & objList) override;

    // puts
    virtual Error putBlobMetaDesc(const std::string & blobName,
            const BlobMetaDesc & blobMeta) override;

    virtual Error putObject(const std::string & blobName, fds_uint64_t offset,
            const ObjectID & obj) override;

    virtual Error putObject(const std::string & blobName, const BlobObjList & objs) override;

    virtual Error putBatch(const std::string & blobName, const BlobMetaDesc & blobMeta,
            const BlobObjList & puts, const std::vector<fds_uint64_t> & deletes) override;

    virtual Error putBatch(const std::string & blobName, const BlobMetaDesc & blobMeta,
            CatWriteBatch & wb) override;

    // deletes
    virtual Error deleteObject(const std::string & blobName, fds_uint64_t offset) override;

    virtual Error deleteObject(const std::string & blobName, fds_uint64_t startOffset,
            fds_uint64_t endOffset) override;

    virtual Error deleteBlobMetaDesc(const std::string & blobName);

  private:
    // methods
    inline Catalog::catalog_iterator_t * getSnapshotIter(Catalog::catalog_roptions_t& opts) {
        catalog_->GetSnapshot(opts);
        return catalog_->NewIterator(opts);
    }

    // vars

    // Catalog that stores volume's objects
    Catalog * catalog_;
    BlobObjKeyComparator cmp_;

    // as the configuration will not be refreshed frequently, we can read it without lock
    FdsConfigAccessor configHelper_;

    std::string timelineDir_;
};
}  // namespace fds
#endif  // SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMPERSISTVOLDB_H_
