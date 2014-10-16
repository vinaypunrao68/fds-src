/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_ACCESS_MGR_INCLUDE_STORHVVOLUMES_H_
#define SOURCE_ACCESS_MGR_INCLUDE_STORHVVOLUMES_H_

#include <string>
#include <vector>
#include <map>
#include <boost/thread/thread.hpp>
#include <boost/lockfree/queue.hpp>
#include <iostream>
#include <boost/atomic.hpp>
#include <concurrency/ThreadPool.h>

#include <unordered_map>

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


/* defaults */
#define FDS_DEFAULT_VOL_UUID 1


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
class AmQosReq;

class StorHvVolumeTable : public HasLogger {
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

struct TxnRequest {
    BlobTxId::ptr txDesc;

    void setBlobTxId(BlobTxId::ptr txDesc) {
        this->txDesc = txDesc;
    }
};

class AbortBlobTxReq : public FdsBlobReq {
  public:
    BlobTxId::ptr txDesc;

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
            txDesc(_txDesc) {
        setVolumeName(_vol_name);
        e2eReqPerfCtx.type = AM_ABORT_BLOB_OBJ_REQ;
        e2eReqPerfCtx.name = "volume:" + std::to_string(_volid);
        e2eReqPerfCtx.reset_volid(_volid);

        fds::PerfTracer::tracePointBegin(e2eReqPerfCtx);
    }
    virtual ~AbortBlobTxReq() {
        fds::PerfTracer::tracePointEnd(e2eReqPerfCtx);
    }

    BlobTxId::const_ptr getTxId() const {
        return txDesc;
    }
};

class CommitBlobTxReq : public FdsBlobReq {
  public:
    BlobTxId::ptr txDesc;
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
            txDesc(_txDesc) {
        setVolumeName(_vol_name);
        e2eReqPerfCtx.type = AM_COMMIT_BLOB_OBJ_REQ;
        e2eReqPerfCtx.name = "volume:" + std::to_string(_volid);
        e2eReqPerfCtx.reset_volid(_volid);

        fds::PerfTracer::tracePointBegin(e2eReqPerfCtx);
    }
    virtual ~CommitBlobTxReq() {
        fds::PerfTracer::tracePointEnd(e2eReqPerfCtx);
    }

    BlobTxId::const_ptr getTxId() const {
        return txDesc;
    }
};

class StartBlobTxReq : public FdsBlobReq {
  public:
    std::string  volumeName;
    fds_int32_t  blobMode;
    BlobTxId     txId;
    fds_uint64_t dmtVersion;

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
        setVolumeName(_vol_name);
        e2eReqPerfCtx.type = AM_START_BLOB_OBJ_REQ;
        e2eReqPerfCtx.name = "volume:" + std::to_string(_volid);
        e2eReqPerfCtx.reset_volid(_volid);

        fds::PerfTracer::tracePointBegin(e2eReqPerfCtx);
    }

    virtual ~StartBlobTxReq() {
        fds::PerfTracer::tracePointEnd(e2eReqPerfCtx);
    }

    fds_int32_t getBlobMode() const {
        return blobMode;
    }
};

class StatBlobReq : public FdsBlobReq {
  public:
    typedef std::function<void (const Error&)> GetBlobProcCb;

    GetBlobProcCb processorCb;
    fds_volid_t base_vol_id;

    /**
     * Request constructor. Some of the fields
     * are not actually needed...the base blob
     * request class just expects them.
     */
    StatBlobReq(fds_volid_t          _volid,
                const std::string   &_vol_name,
                const std::string   &_blob_name,
                fds_uint64_t         _blob_offset,
                fds_uint64_t         _data_len,
                char                *_data_buf,
                CallbackPtr cb) :
            FdsBlobReq(FDS_STAT_BLOB, _volid, _blob_name, _blob_offset,
                       _data_len, _data_buf, cb) {
        setVolumeName(_vol_name);
        e2eReqPerfCtx.type = AM_STAT_BLOB_OBJ_REQ;
        e2eReqPerfCtx.name = "volume:" + std::to_string(_volid);
        e2eReqPerfCtx.reset_volid(_volid);

        fds::PerfTracer::tracePointBegin(e2eReqPerfCtx);
    }

    StatBlobReq(fds_volid_t          _volid,
                const std::string   &_vol_name,
                const std::string   &_blob_name,
                CallbackPtr cb) :
            FdsBlobReq(FDS_STAT_BLOB, _volid, _blob_name, 0,
                       0, NULL, cb) {
        setVolumeName(_vol_name);
        e2eReqPerfCtx.type = AM_STAT_BLOB_OBJ_REQ;
        e2eReqPerfCtx.name = "volume:" + std::to_string(volId);
        e2eReqPerfCtx.reset_volid(volId);

        fds::PerfTracer::tracePointBegin(e2eReqPerfCtx);
    }

    virtual ~StatBlobReq() {
        fds::PerfTracer::tracePointEnd(e2eReqPerfCtx);
    }
};

struct SetBlobMetaDataReq : FdsBlobReq {
  public:
    BlobTxId::ptr txDesc;
    boost::shared_ptr<FDSP_MetaDataList> metaDataList;
    SetBlobMetaDataReq(fds_volid_t _volid,
                       const std::string   &_vol_name,
                       const std::string   &_blob_name,
                       BlobTxId::ptr _txDesc,
                       boost::shared_ptr<FDSP_MetaDataList> _metaDataList,
                       CallbackPtr cb) :
            FdsBlobReq(FDS_SET_BLOB_METADATA, _volid, _blob_name, 0, 0, NULL, cb),
            txDesc(_txDesc),  metaDataList(_metaDataList) {
        setVolumeName(_vol_name);
        e2eReqPerfCtx.type = AM_SET_BLOB_META_OBJ_REQ;
        e2eReqPerfCtx.name = "volume:" + std::to_string(volId);
        e2eReqPerfCtx.reset_volid(volId);

        fds::PerfTracer::tracePointBegin(e2eReqPerfCtx);
    }

    virtual ~SetBlobMetaDataReq() {
        fds::PerfTracer::tracePointEnd(e2eReqPerfCtx);
    }

    boost::shared_ptr<FDSP_MetaDataList> getMetaDataListPtr()
            const { return metaDataList; }

    BlobTxId::const_ptr getTxId() const {
        return txDesc;
    }
};

struct GetVolumeMetaDataReq : FdsBlobReq {
    GetVolumeMetaDataReq(fds_volid_t volId, const std::string & volumeName, CallbackPtr cb) :
            FdsBlobReq(FDS_GET_VOLUME_METADATA, volId, "" , 0, 0, NULL, cb) {
        setVolumeName(volumeName);
        e2eReqPerfCtx.type = AM_GET_VOLUME_META_OBJ_REQ;
        e2eReqPerfCtx.name = "volume:" + std::to_string(volId);
        e2eReqPerfCtx.reset_volid(volId);

        fds::PerfTracer::tracePointBegin(e2eReqPerfCtx);
    }

    virtual ~GetVolumeMetaDataReq() {
        fds::PerfTracer::tracePointEnd(e2eReqPerfCtx);
    }

    /// Metadata to be returned
    fpi::FDSP_VolumeMetaData volumeMetadata;

    typedef std::function<void (const Error&)> GetVolMetadataProcCb;
    GetVolMetadataProcCb processorCb;
};

struct GetBlobMetaDataReq : FdsBlobReq {
    GetBlobMetaDataReq(fds_volid_t volId, const std::string & volumeName,
                       const std::string &_blob_name, CallbackPtr cb) :
            FdsBlobReq(FDS_GET_BLOB_METADATA, volId, _blob_name , 0, 0, NULL, cb) {
        setVolumeName(volumeName);
        e2eReqPerfCtx.type = AM_GET_BLOB_META_OBJ_REQ;
        e2eReqPerfCtx.name = "volume:" + std::to_string(volId);
        e2eReqPerfCtx.reset_volid(volId);

        fds::PerfTracer::tracePointBegin(e2eReqPerfCtx);
    }

    virtual ~GetBlobMetaDataReq() {
        fds::PerfTracer::tracePointEnd(e2eReqPerfCtx);
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
        setVolumeName(_vol_name);
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
    BlobTxId::ptr txDesc;
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
        fds_verify(txDesc == NULL);
        txDesc = BlobTxId::ptr(new BlobTxId(txId));
    }

    BlobTxId::const_ptr getTxId() const {
        return txDesc;
    }

    virtual ~PutBlobReq();

    void DoCallback(FDSN_Status status, ErrorDetails* errDetails) {
        (putObjCallback)(req_context, dataLen, blobOffset, dataBuf,
                         callback_data, status, errDetails);
    }

    void notifyResponse(fds::AmQosReq* qosReq, const Error &e);
    void notifyResponse(StorHvQosCtrl *qos_ctrl, fds::AmQosReq* qosReq, const Error &e);
};


struct DeleteBlobReq: FdsBlobReq, TxnRequest {
    typedef std::function<void (const Error&)> DeleteBlobProcCb;

    BucketContext *bucket_ctxt;
    std::string ObjKey;
    void *req_context;
    fdsnResponseHandler responseCallback;
    void *callback_data;

    DeleteBlobProcCb processorCb;
    fds_volid_t base_vol_id;

    DeleteBlobReq(fds_volid_t _volid,
                  const std::string& _blob_name,
                  BucketContext* _bucket_ctxt,
                  void* _req_context,
                  fdsnResponseHandler _resp_handler,
                  void* _callback_data)
            : FdsBlobReq(FDS_DELETE_BLOB, _volid, _blob_name, 0, 0, NULL,
                         FDS_NativeAPI::DoCallback, this, Error(ERR_OK), 0),
              bucket_ctxt(_bucket_ctxt),
              ObjKey(_blob_name),
              req_context(_req_context),
              responseCallback(_resp_handler),
              callback_data(_callback_data) {
        e2eReqPerfCtx.type = AM_DELETE_OBJ_REQ;
        e2eReqPerfCtx.name = "volume:" + std::to_string(volId);
        e2eReqPerfCtx.reset_volid(volId);
        qosPerfCtx.type = AM_DELETE_QOS;
        qosPerfCtx.name = "volume:" + std::to_string(volId);
        qosPerfCtx.reset_volid(volId);
        hashPerfCtx.type = AM_DELETE_HASH;
        hashPerfCtx.name = "volume:" + std::to_string(volId);
        hashPerfCtx.reset_volid(volId);
        dmPerfCtx.type = AM_DELETE_DM;
        dmPerfCtx.name = "volume:" + std::to_string(volId);
        dmPerfCtx.reset_volid(volId);
        smPerfCtx.type = AM_DELETE_SM;
        smPerfCtx.name = "volume:" + std::to_string(volId);
        smPerfCtx.reset_volid(volId);

        fds::PerfTracer::tracePointBegin(e2eReqPerfCtx);
    }

    DeleteBlobReq(fds_volid_t _volid,
                  const std::string& _blob_name,
                  const std::string& volumeName,
                  CallbackPtr cb)
            : FdsBlobReq(FDS_DELETE_BLOB, _volid,
                         _blob_name, 0, 0, NULL, cb) {
        setVolumeName(volumeName);

        e2eReqPerfCtx.type = AM_DELETE_OBJ_REQ;
        e2eReqPerfCtx.name = "volume:" + std::to_string(volId);
        e2eReqPerfCtx.reset_volid(volId);
        qosPerfCtx.type = AM_DELETE_QOS;
        qosPerfCtx.name = "volume:" + std::to_string(volId);
        qosPerfCtx.reset_volid(volId);
        hashPerfCtx.type = AM_DELETE_HASH;
        hashPerfCtx.name = "volume:" + std::to_string(volId);
        hashPerfCtx.reset_volid(volId);
        dmPerfCtx.type = AM_DELETE_DM;
        dmPerfCtx.name = "volume:" + std::to_string(volId);
        dmPerfCtx.reset_volid(volId);
        smPerfCtx.type = AM_DELETE_SM;
        smPerfCtx.name = "volume:" + std::to_string(volId);
        smPerfCtx.reset_volid(volId);

        fds::PerfTracer::tracePointBegin(e2eReqPerfCtx);
    }

    virtual ~DeleteBlobReq() {
        fds::PerfTracer::tracePointEnd(e2eReqPerfCtx);
    }

    void DoCallback(FDSN_Status status, ErrorDetails* errDetails) {
        (responseCallback)(status, errDetails, callback_data);
    }
};


#define FDS_SH_BUCKET_LIST_MAGIC  0xBCE12345
#define FDS_SH_BUCKET_STATS_MAGIC 0xCDF23456
class BucketStatsReq: public FdsBlobReq {
  public:
    void *request_context;
    fdsnBucketStatsHandler resp_handler;
    void *callback_data;

    /* for "get all bucket stats", blob name in base class is set to
     * "all", it's ok if some other bucket will have this name, because
     * we can identify response to a request by trans id (once request is dispatched */

    BucketStatsReq(void *_req_context,
                   fdsnBucketStatsHandler _handler,
                   void *_callback_data)
            : FdsBlobReq(FDS_BUCKET_STATS, admin_vol_id, "all", FDS_SH_BUCKET_STATS_MAGIC, 0, NULL,
                         FDS_NativeAPI::DoCallback, this, Error(ERR_OK), 0),
              request_context(_req_context),
              resp_handler(_handler),
              callback_data(_callback_data) {
    }

    ~BucketStatsReq() {}

    void DoCallback(const std::string& timestamp,
                    int content_count,
                    const BucketStatsContent* contents,
                    FDSN_Status status,
                    ErrorDetails *err_details) {
        (resp_handler)(timestamp, content_count, contents,
                       request_context, callback_data, status, err_details);
    }
};

class ListBucketReq: public FdsBlobReq {
  public:
    BucketContext *bucket_ctxt;
    std::string prefix;
    std::string marker;
    std::string delimiter;
    fds_uint32_t maxkeys;
    void *request_context;
    fdsnListBucketHandler handler;
    void *callback_data;
    fds_long_t iter_cookie = 0;

    /* sets bucket name to blob name in the base class,
     * which is used to get trans id in journal table, and
     * some magic number for offset */

    ListBucketReq(fds_volid_t _volid,
                  BucketContext *_bucket_ctxt,
                  const std::string& _prefix,
                  const std::string& _marker,
                  const std::string& _delimiter,
                  fds_uint32_t _max_keys,
                  void* _req_context,
                  fdsnListBucketHandler _handler,
                  void* _callback_data)
            : FdsBlobReq(FDS_LIST_BUCKET, _volid,
                         _bucket_ctxt->bucketName, FDS_SH_BUCKET_LIST_MAGIC, 0, NULL,
                         FDS_NativeAPI::DoCallback, this, Error(ERR_OK), 0),
        bucket_ctxt(_bucket_ctxt),
        prefix(_prefix),
        marker(_marker),
        delimiter(_delimiter),
        maxkeys(_max_keys),
        request_context(_req_context),
        handler(_handler),
        callback_data(_callback_data),
        iter_cookie(0) {
    }

    ListBucketReq(fds_volid_t _volid,
                  BucketContext *_bucket_ctxt,
                  fds_uint32_t _max_keys,
                  CallbackPtr cb)
            : FdsBlobReq(FDS_LIST_BUCKET, _volid,
                         _bucket_ctxt->bucketName, 0, 0, NULL, cb), bucket_ctxt(_bucket_ctxt) {
    }

    ~ListBucketReq() {}

    void DoCallback(int isTruncated,
                    const char* next_marker,
                    int contents_count,
                    const ListBucketContents* contents,
                    FDSN_Status status,
                    ErrorDetails* errDetails) {
        (handler)(isTruncated, next_marker, contents_count,
                  contents, 0, NULL, callback_data, status);
    }
};

/*
 * Internal wrapper class for AM QoS
 * requests.
 */
class AmQosReq : public FDS_IOType {
  private:
    FdsBlobReq *blobReq;

  public:
    AmQosReq(FdsBlobReq *_br,
             fds_uint32_t _reqId)
            : blobReq(_br) {
        /*
         * Set the base class defaults
         */
        io_magic  = FDS_SH_IO_MAGIC_IN_USE;
        io_module = STOR_HV_IO;
        io_vol_id = blobReq->getVolId();
        io_type   = blobReq->getIoType();
        io_req_id = _reqId;
    }
    ~AmQosReq() {
    }

    fds_bool_t magicInUse() const {
        return blobReq->magicInUse();
    }
    FdsBlobReq* getBlobReqPtr() {
        return blobReq;
    }
    void setVolId(fds_volid_t volid) {
        io_vol_id = volid;
        blobReq->setVolId(volid);
    }
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_STORHVVOLUMES_H_
