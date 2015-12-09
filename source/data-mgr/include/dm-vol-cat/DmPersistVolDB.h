/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMPERSISTVOLDB_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMPERSISTVOLDB_H_

// Standard includes.
#include <map>
#include <string>
#include <vector>
#include <atomic>
// Internal includes.
#include "catalogKeys/CatalogKeyComparator.h"
#include "concurrency/RwLock.h"
#include "dm-vol-cat/DmPersistVolCat.h"
#include "lib/Catalog.h"
#include "fds_config.hpp"
#include "fds_process.h"

namespace fds {

class DmPersistVolDB : public HasLogger, public DmPersistVolCat {
  public:
    // types
    typedef boost::shared_ptr<DmPersistVolDB> ptr;
    typedef boost::shared_ptr<const DmPersistVolDB> const_ptr;

    static const std::string CATALOG_WRITE_BUFFER_SIZE_STR;
    static const std::string CATALOG_CACHE_SIZE_STR;
    static const std::string CATALOG_MAX_LOG_FILES_STR;
    static const std::string ENABLE_TIMELINE_STR;

    // ctor & dtor
    DmPersistVolDB(fds_volid_t volId,
                   fds_uint32_t objSize,
                   fds_bool_t snapshot,
                   fds_bool_t readOnly,
                   fds_bool_t clone,
                   fds_volid_t srcVolId = invalid_vol_id)
            : DmPersistVolCat(volId,
                              objSize,
                              snapshot,
                              readOnly,
                              clone,
                              fpi::FDSP_VOL_S3_TYPE,
                              srcVolId),
        configHelper_(g_fdsprocess->get_conf_helper()), snapshotCount(0)
    {
        const FdsRootDir* root = g_fdsprocess->proc_fdsroot();
        timelineDir_ = root->dir_timeline_dm() + getVolIdStr() + "/";
    }
    virtual ~DmPersistVolDB();

    // methods
    virtual Error activate() override;

    virtual Error copyVolDir(const std::string & destName) override;

    // gets
    virtual Error getVolumeMetaDesc(VolumeMetaDesc & volDesc) override;

    virtual Error getBlobMetaDesc(const std::string & blobName,
                                  BlobMetaDesc & blobMeta,
                                  Catalog::MemSnap m = NULL) override;

    virtual Error getAllBlobMetaDesc(std::vector<BlobMetaDesc> & blobMetaList) override;

    virtual Error getBlobMetaDescForPrefix (std::string const& prefix,
                                            std::string const& delimiter,
                                            std::vector<BlobMetaDesc>& blobMetaList,
                                            std::vector<std::string>& skippedPrefixes) override;

    virtual Error getObject(const std::string & blobName, fds_uint64_t offset,
            ObjectID & obj) override;

    virtual Error getObject(const std::string & blobName, fds_uint64_t startOffset,
                            fds_uint64_t endOffset,
                            fpi::FDSP_BlobObjectList& objList,
                            const Catalog::MemSnap snap = NULL) override;

    virtual Error getObject(const std::string & blobName, fds_uint64_t startOffset,
            fds_uint64_t endOffset, BlobObjList & objList) override;

    virtual Error getLatestSequenceId(blob_version_t & max) override;

    virtual Error getAllBlobsWithSequenceId(std::map<std::string, int64_t>& blobsSeqId,
														Catalog::MemSnap snap) override;

    virtual Error getInMemorySnapshot(Catalog::MemSnap &snap) override;

    virtual void getObjectIds(const uint32_t &maxObjs,
                              const Catalog::MemSnap &snap,
                              std::unique_ptr<Catalog::catalog_iterator_t>& dbItr,
                              std::list<ObjectID> &objects) override;

    // puts
    virtual Error putVolumeMetaDesc(const VolumeMetaDesc & volDesc) override;

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

    virtual Error deleteBlobMetaDesc(const std::string & blobName) override;
    virtual void forEachObject(std::function<void(const ObjectID&)>) override;

    virtual Error freeInMemorySnapshot(Catalog::MemSnap& snap) override;
    virtual uint64_t getNumInMemorySnapshots() override;

    Catalog* getCatalog() {
        return catalog_.get();
    }

  private:
    // methods

    // vars
    std::atomic<uint64_t> snapshotCount;
    // Catalog that stores volume's objects
    std::unique_ptr<Catalog> catalog_;
    CatalogKeyComparator cmp_;

    // as the configuration will not be refreshed frequently, we can read it without lock
    FdsConfigAccessor configHelper_;

    std::string timelineDir_;
};
}  // namespace fds
#endif  // SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMPERSISTVOLDB_H_
