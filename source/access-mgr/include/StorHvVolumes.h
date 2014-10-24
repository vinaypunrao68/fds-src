/*
 * Copyright 2013-2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_STORHVVOLUMES_H_
#define SOURCE_ACCESS_MGR_INCLUDE_STORHVVOLUMES_H_

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <unordered_map>
#include <boost/atomic.hpp>
#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include <concurrency/ThreadPool.h>

#include <fds_error.h>
#include <fds_types.h>
#include <fds_volume.h>
#include <qos_ctrl.h>
#include <blob/BlobTypes.h>
#include <fds_qos.h>
#include <util/Log.h>
#include <concurrency/RwLock.h>
#include <VolumeCatalogCache.h>
#include <StorHvJournal.h>
#include <native_api.h>
#include "PerfTrace.h"

#include "AmQosReq.h"
#include "FdsBlobReq.h"


/*
 * Forward declaration of SH control class
 */
class StorHvCtrl;

namespace fds {

/* Forward declarations */
class StorHvQosCtrl;

class StorHvVolume : public FDS_Volume , public HasLogger
{
  public:
    StorHvVolume(const VolumeDesc& vdesc, StorHvCtrl *sh_ctrl, fds_log *parent_log);
    ~StorHvVolume();

    /* safe destruction, after this, volume object is not valid */
    void destroy();

    bool isValidLocked() const;

    /* read locks. Journal table and volume catalog cache have their own locks,
     * so exposing read lock to protect against volume being destroyed
     */
    void readLock();
    void readUnlock();

  public:
    StorHvJournal *journal_tbl;
    VolumeCatalogCache *vol_catalog_cache;
    int blkdev_minor;
    /* Reference to parent SH instance */
    StorHvCtrl *parent_sh;

    /*
     * per volume queue
     */
    FDS_VolumeQueue*  volQueue;

  private:
    /* lock to prevent volume destruction while accessing volume data */
    fds_rwlock rwlock;
    bool is_valid;
};

typedef std::vector<fds_volid_t> StorHvVolVec;

class StorHvVolumeLock
{
  public:
    /* Ok if vol == NULL, will not do anything */
    explicit StorHvVolumeLock(StorHvVolume *vol);
    ~StorHvVolumeLock();

  private:
    StorHvVolume *shvol;
};

class StorHvVolumeTable : public HasLogger {
    static const fds_volid_t fds_default_vol_uuid = 1;

  public:
    /// A logger is created if not passed in
    explicit StorHvVolumeTable(StorHvCtrl *sh_ctrl);
    /// Use logger that passed in to the constructor
    StorHvVolumeTable(StorHvCtrl *sh_ctrl, fds_log *parent_log);
    ~StorHvVolumeTable();

    Error registerVolume(const VolumeDesc& vdesc);
    Error removeVolume(fds_volid_t vol_uuid);

    /**
     * Returns the locked volume object. Guarantees that the
     * returned volume object is valid (i.e., can safely access
     * journal table and volume catalog cache)
     * Must call StorHvVolume::readUnlock on returned volume object
     * Returns NULL if volume does not exist
     */
    StorHvVolume* getLockedVolume(fds_volid_t vol_uuid);

    /**
     * Returns volume but not thread-safe
     * Use StorHvVolumeLock to lock the volume and check if volume
     * object is still valid via StorHvVolume::isValidLocked()
     * before using the volume object
     * Returns NULL is volume does not exist
<     */
    StorHvVolume* getVolume(fds_volid_t vol_uuid);

    /**
     * Returns list of volume ids currently in the table
     */
    StorHvVolVec getVolumeIds();

    /**
     * Returns the volumes max object size
     */
    fds_uint32_t getVolMaxObjSize(fds_volid_t volUuid);

    /**
     * Returns volume uuid if found in volume map.
     * if volume does not exist, returns 'invalid_vol_id'
     */
    fds_volid_t getVolumeUUID(const std::string& vol_name);

    /**
     * Returns the base volume id for DMT lookup.
     * for regular volumes , returns the same uuid
     * for snapshots, returns the parent uuid
     */
    fds_volid_t getBaseVolumeId(fds_volid_t volId);

    /**
     * Returns volume name if found in volume map.
     * if volume does not exist, returns empty string
     */
    std::string getVolumeName(fds_volid_t volId);

    /** returns true if volume exists, otherwise retuns false */
    fds_bool_t volumeExists(const std::string& vol_name);

    /**
     * Add blob request to wait queue -- those are blobs that
     * are waiting for OM to attach buckets to AM; once
     * vol table receives vol attach event, it will move
     * all requests waiting in the queue for that bucket to
     * appropriate qos queue
     */
    void addBlobToWaitQueue(const std::string& bucket_name,
                            FdsBlobReq* blob_req);

    /**
     * Complete blob requests that are waiting in wait queue
     * with error (because e.g. volume not found, etc).
     */
    void completeWaitBlobsWithError(const std::string& bucket_name,
                                    Error err) {
        moveWaitBlobsToQosQueue(invalid_vol_id, bucket_name, err);
    }

    Error modifyVolumePolicy(fds_volid_t vol_uuid,
                             const VolumeDesc& vdesc);
    void moveWaitBlobsToQosQueue(fds_volid_t vol_uuid,
                                 const std::string& vol_name,
                                 Error error);

  private:
    /// print volume map, other detailed state to log
    void dump();

  private:
    /// volume uuid -> StorHvVolume map
    std::unordered_map<fds_volid_t, StorHvVolume*> volume_map;

    /// Protects volume_map
    fds_rwlock map_rwlock;

    /**
     * list of blobs that are waiting for OM to attach appropriate
     * bucket to AM if it exists/ or return 'does not exist error
     */
    typedef std::vector<AmQosReq*> bucket_wait_vec_t;
    typedef std::map<std::string, bucket_wait_vec_t> wait_blobs_t;
    typedef std::map<std::string, bucket_wait_vec_t>::iterator wait_blobs_it_t;
    wait_blobs_t wait_blobs;
    fds_rwlock wait_rwlock;

    /// Reference to parent SH instance
    StorHvCtrl *parent_sh;
};

class AbortBlobTxReq : public FdsBlobReq {
  public:
    BlobTxId::ptr tx_desc;

    typedef std::function<void (const Error&)> AbortBlobTxProcCb;
    AbortBlobTxProcCb processorCb;

    /**
     * Request constructor. Some of the fields
     * are not actually needed...the base blob
     * request class just expects them.
     */
    AbortBlobTxReq(fds_volid_t        _volid,
                   const std::string &_vol_name,
                   const std::string &_blob_name,
                   BlobTxId::ptr _txDesc,
                   CallbackPtr        _cb) :
            FdsBlobReq(FDS_ABORT_BLOB_TX, _volid, _blob_name, 0, 0, 0, _cb),
            tx_desc(_txDesc) {
        volume_name = _vol_name;
        e2e_req_perf_ctx.type = AM_ABORT_BLOB_OBJ_REQ;
        e2e_req_perf_ctx.name = "volume:" + std::to_string(_volid);
        e2e_req_perf_ctx.reset_volid(_volid);

        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }
    virtual ~AbortBlobTxReq() {
        fds::PerfTracer::tracePointEnd(e2e_req_perf_ctx);
    }
};

class CommitBlobTxReq : public FdsBlobReq {
  public:
    BlobTxId::ptr tx_desc;

    typedef std::function<void (const Error&)> CommitBlobProcCb;
    CommitBlobProcCb processorCb;

    /**
     * Request constructor. Some of the fields
     * are not actually needed...the base blob
     * request class just expects them.
     */
    CommitBlobTxReq(fds_volid_t        _volid,
                   const std::string &_vol_name,
                   const std::string &_blob_name,
                   BlobTxId::ptr _txDesc,
                   CallbackPtr        _cb) :
            FdsBlobReq(FDS_COMMIT_BLOB_TX, _volid, _blob_name, 0, 0, 0, _cb),
            tx_desc(_txDesc) {
        volume_name = _vol_name;
        e2e_req_perf_ctx.type = AM_COMMIT_BLOB_OBJ_REQ;
        e2e_req_perf_ctx.name = "volume:" + std::to_string(_volid);
        e2e_req_perf_ctx.reset_volid(_volid);

        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }
    virtual ~CommitBlobTxReq() {
        fds::PerfTracer::tracePointEnd(e2e_req_perf_ctx);
    }
};

class StartBlobTxReq : public FdsBlobReq {
  public:
    std::string     volumeName;
    fds_int32_t     blobMode;
    BlobTxId::ptr   tx_desc;
    fds_uint64_t    dmtVersion;

    typedef std::function<void (const Error&)> StartBlobTxProcCb;
    StartBlobTxProcCb processorCb;

    /**
     * Request constructor. Some of the fields
     * are not actually needed...the base blob
     * request class just expects them.
     */
    StartBlobTxReq(fds_volid_t        _volid,
                   const std::string &_vol_name,
                   const std::string &_blob_name,
                   const int32_t _blob_mode,
                   CallbackPtr        _cb) :
            FdsBlobReq(FDS_START_BLOB_TX, _volid, _blob_name, 0, 0, 0, _cb),
            volumeName(_vol_name), blobMode(_blob_mode) {
        volume_name = _vol_name;
        e2e_req_perf_ctx.type = AM_START_BLOB_OBJ_REQ;
        e2e_req_perf_ctx.name = "volume:" + std::to_string(_volid);
        e2e_req_perf_ctx.reset_volid(_volid);

        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }

    virtual ~StartBlobTxReq() {
        fds::PerfTracer::tracePointEnd(e2e_req_perf_ctx);
    }

    fds_int32_t getBlobMode() const {
        return blobMode;
    }
};

struct SetBlobMetaDataReq : FdsBlobReq {
  public:
    typedef std::function<void (const Error&)> SetBlobMetadataProcCb;

    BlobTxId::ptr tx_desc;
    boost::shared_ptr<FDSP_MetaDataList> metaDataList;

    SetBlobMetadataProcCb processorCb;
    fds_uint64_t dmt_version;

    SetBlobMetaDataReq(fds_volid_t _volid,
                       const std::string   &_vol_name,
                       const std::string   &_blob_name,
                       BlobTxId::ptr _txDesc,
                       boost::shared_ptr<FDSP_MetaDataList> _metaDataList,
                       CallbackPtr cb) :
            FdsBlobReq(FDS_SET_BLOB_METADATA, _volid, _blob_name, 0, 0, NULL, cb),
            tx_desc(_txDesc),  metaDataList(_metaDataList) {
        volume_name = _vol_name;
        e2e_req_perf_ctx.type = AM_SET_BLOB_META_OBJ_REQ;
        e2e_req_perf_ctx.name = "volume:" + std::to_string(vol_id);
        e2e_req_perf_ctx.reset_volid(vol_id);

        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }

    virtual ~SetBlobMetaDataReq() {
        fds::PerfTracer::tracePointEnd(e2e_req_perf_ctx);
    }

    boost::shared_ptr<FDSP_MetaDataList> getMetaDataListPtr()
            const { return metaDataList; }
};

struct GetBlobMetaDataReq : FdsBlobReq {
    GetBlobMetaDataReq(fds_volid_t volId, const std::string & volumeName,
                       const std::string &_blob_name, CallbackPtr cb) :
            FdsBlobReq(FDS_GET_BLOB_METADATA, volId, _blob_name , 0, 0, NULL, cb) {
        volume_name = volumeName;
        e2e_req_perf_ctx.type = AM_GET_BLOB_META_OBJ_REQ;
        e2e_req_perf_ctx.name = "volume:" + std::to_string(volId);
        e2e_req_perf_ctx.reset_volid(volId);

        fds::PerfTracer::tracePointBegin(e2e_req_perf_ctx);
    }

    virtual ~GetBlobMetaDataReq() {
        fds::PerfTracer::tracePointEnd(e2e_req_perf_ctx);
    }
};


/**
 * AM request to locally attach a volume.
 * This request is not specific to a blob,
 * but needs to be in the blob wait queue
 * to call the callback and notify that the
 * attach is complete.
 */
class AttachVolBlobReq : public FdsBlobReq {
  public:
    /**
     * Request constructor. Some of the fields
     * are not actually needed...the base blob
     * request class just expects them.
     */
    AttachVolBlobReq(fds_volid_t          _volid,
                     const std::string   &_vol_name,
                     const std::string   &_blob_name,
                     fds_uint64_t         _blob_offset,
                     fds_uint64_t         _data_len,
                     char                *_data_buf,
                     CallbackPtr cb) :
            FdsBlobReq(FDS_ATTACH_VOL, _volid, _blob_name, _blob_offset,
                       _data_len, _data_buf, cb) {
        volume_name = _vol_name;
    }
    ~AttachVolBlobReq() {
    }
};

class GetBlobReq: public FdsBlobReq {
  public:
    typedef std::function<void (const Error&)> GetBlobProcCb;
    GetBlobProcCb processorCb;

    fds_volid_t base_vol_id;

    GetBlobReq(fds_volid_t _volid,
               const std::string& _volumeName,
               const std::string& _blob_name,
               fds_uint64_t _blob_offset,
               fds_uint64_t _data_len,
               char* _data_buf,
               fds_uint64_t _byte_count,
               CallbackPtr cb);

    virtual ~GetBlobReq();

    void DoCallback(FDSN_Status status, ErrorDetails* errDetails) {
        fds_panic("Depricated. You shouldn't be here!");
    }
};


class PutBlobReq: public FdsBlobReq {
  public:
    // TODO(Andrew): Fields that could use some cleanup.
    // We can mostly remove these with the new callback mechanism
    BucketContext *bucket_ctxt;
    std::string ObjKey;
    PutPropertiesPtr putProperties;
    void *req_context;
    void *callback_data;
    fds_bool_t lastBuf;
    fdsnPutObjectHandler putObjCallback;

    // Needed fields
    BlobTxId::ptr tx_desc;
    fds_uint64_t dmtVersion;

    /// Used for putBlobOnce scenarios.
    boost::shared_ptr< std::map<std::string, std::string> > metadata;
    fds_int32_t blobMode;

    /* ack cnt for responses, decremented when response from SM and DM come back */
    std::atomic<int> respAcks;
    /* Return status for completion callback */
    Error retStatus;

    typedef std::function<void (const Error&)> UpdateBlobProcCb;
    UpdateBlobProcCb processorCb;

    /// Constructor used on regular putBlob requests.
    PutBlobReq(fds_volid_t _volid,
               const std::string& _volumeName,
               const std::string& _blob_name,
               fds_uint64_t _blob_offset,
               fds_uint64_t _data_len,
               char* _data_buf,
               BlobTxId::ptr _txDesc,
               fds_bool_t _last_buf,
               BucketContext* _bucket_ctxt,
               PutPropertiesPtr _put_props,
               void* _req_context,
               CallbackPtr _cb);

    /// Constructor used on putBlobOnce requests.
    PutBlobReq(fds_volid_t          _volid,
               const std::string&   _volumeName,
               const std::string&   _blob_name,
               fds_uint64_t         _blob_offset,
               fds_uint64_t         _data_len,
               char*                _data_buf,
               fds_int32_t          _blobMode,
               boost::shared_ptr< std::map<std::string, std::string> >& _metadata,
               CallbackPtr _cb);

    fds_bool_t isLastBuf() const {
        return lastBuf;
    }

    std::string getEtag() const {
        std::string etag = "";
        if (putProperties != NULL) {
            etag = putProperties->md5;
        }
        return etag;
    }

    void setTxId(const BlobTxId &txId) {
        // We only expect to need to set this in putBlobOnce cases
        fds_verify(tx_desc == NULL);
        tx_desc = BlobTxId::ptr(new BlobTxId(txId));
    }

    virtual ~PutBlobReq();

    void DoCallback(FDSN_Status status, ErrorDetails* errDetails) {
        (putObjCallback)(req_context, data_len, blob_offset, data_buf,
                         callback_data, status, errDetails);
    }

    void notifyResponse(fds::AmQosReq* qosReq, const Error &e);
    void notifyResponse(StorHvQosCtrl *qos_ctrl, fds::AmQosReq* qosReq, const Error &e);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_STORHVVOLUMES_H_
