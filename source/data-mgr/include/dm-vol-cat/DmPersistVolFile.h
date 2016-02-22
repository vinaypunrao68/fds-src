/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMPERSISTVOLFILE_H_
#define SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMPERSISTVOLFILE_H_

#include <string>
#include <vector>

#include <fds_process.h>
#include <fds_module.h>
#include <serialize.h>

#include <cache/SharedKvCache.h>

#include <dm-vol-cat/DmPersistVolCat.h>
#include <dm-vol-cat/DmOIDArrayMmap.h>

namespace fds {

class DmPersistVolFile : public HasLogger, public DmPersistVolCat {
  public:
    // types
    typedef boost::shared_ptr<DmPersistVolFile> ptr;
    typedef boost::shared_ptr<const DmPersistVolFile> const_ptr;

    static const fds_uint32_t MMAP_CACHE_SIZE;

    // ctor & dtor
    DmPersistVolFile(CommonModuleProviderIf *modProvider,
                     fds_volid_t volId, fds_uint32_t objSize,
                     fds_bool_t snapshot, fds_bool_t readOnly,
                     fds_volid_t srcVolId = invalid_vol_id);
    virtual ~DmPersistVolFile();

    // methods
    virtual Error activate() override;

    virtual Error copyVolDir(const std::string & destName) override;

    // gets
    virtual Error getBlobMetaDesc(const std::string & blobName,
                                  BlobMetaDesc & blobMeta, Catalog::MemSnap snap) override;

    virtual Error getAllBlobMetaDesc(std::vector<BlobMetaDesc> & blobMetaList) override;

    virtual Error getObject(const std::string & blobName, fds_uint64_t offset,
            ObjectID & obj) override;

    virtual Error getObject(const std::string & blobName, fds_uint64_t startOffset,
                            fds_uint64_t endOffset,
                            fpi::FDSP_BlobObjectList& objList,
                            Catalog::MemSnap snap) override;

    virtual Error getObject(const std::string & blobName, fds_uint64_t startOffset,
            fds_uint64_t endOffset, BlobObjList & objList) override;

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

    virtual bool volSummaryInitialized() override;

    virtual Error initVolSummary(fds_uint64_t logicalSize,
                                 fds_uint64_t blobCount,
                                 fds_uint64_t logicalObjectCount) override;

    virtual Error applyStatDeltas(const fds_uint64_t bytesAdded,
                                  const fds_uint64_t bytesRemoved,
                                  const fds_uint64_t blobsAdded,
                                  const fds_uint64_t blobsRemoved,
                                  const fds_uint64_t objectsAdded,
                                  const fds_uint64_t objectsRemoved) override;

    virtual void resetVolSummary() override;

    virtual Error getVolSummary(fds_uint64_t* logicalSize,
                                fds_uint64_t* blobCount,
                                fds_uint64_t* logicalObjectCount) override;

  private:
    // vars
    int metaFd_;
    int objFd_;
    std::string dirname_;
    std::string objFilename_;
    std::string metaFilename_;

    boost::shared_ptr<SharedKvCache<fds_uint64_t, DmOIDArrayMmap> > mmapCache_;

    boost::shared_ptr<BlobMetaDesc> blobMeta_;  // cached copy
    boost::shared_ptr<serialize::Serializer> s_;
    boost::shared_ptr<serialize::Deserializer> d_;

    fds_mutex cacheLock_;
    fds_mutex metaLock_;

    // methods
    Error getOIDArrayMmap(fds_uint64_t id, boost::shared_ptr<DmOIDArrayMmap> & oidArrayMap,
            fds_bool_t read = true);
};
}  // namespace fds
#endif  // SOURCE_DATA_MGR_INCLUDE_DM_VOL_CAT_DMPERSISTVOLFILE_H_
