/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_SMIO_H_
#define SOURCE_STOR_MGR_INCLUDE_SMIO_H_

/* TODO: move this to interface file in include dir */
#include <lib/OMgrClient.h>

#include <list>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <boost/shared_ptr.hpp>

#include <fdsp/FDSP_types.h>
#include <fds_error.h>
#include <fds_types.h>
#include <fds_volume.h>
#include <TransJournal.h>
#include <leveldb/db.h>
// #include <util/Log.h>
// #include <odb.h>
// #include <qos_ctrl.h>
// #include <util/counter.h>
// #include <ObjStats.h>

// TODO(Rao):
// 1. Remove unnecessary headers
// 2. Move method defs into SmIo.cpp file

namespace fds {
class SmIoReq : public FDS_IOType {
 protected:
    ObjectID     objId;
    // ObjectBuf    objData;
    fds_volid_t  volUuid;
    fds_uint64_t volOffset;
    TransJournalId   transId;
    FDSP_PutObjTypePtr putObjReq;
    FDSP_GetObjTypePtr getObjReq;
    FDSP_DeleteObjTypePtr delObjReq;

 public:
    /*
     * This constructor is generally used for
     * write since it accepts a putObjReq Ptr.
     */
    // TODO(Rao): Deprecate this constructor
    SmIoReq(const std::string& objID,
            // const std::string& _dataStr,
            FDSP_PutObjTypePtr& putObjReq,
            fds_volid_t        _volUuid,
            fds_io_op_t        _ioType,
            fds_uint32_t       _ioReqId) {
        //      objId = ObjectID(_objIdHigh, _objIdLow);
        objId = ObjectID(objID);
        //  memcpy(objId.digest, objID.digest, objId.GetLen());
        // objData.size        = _dataStr.size();
        // objData.data        = _dataStr;
        volUuid             = _volUuid;
        io_vol_id           = volUuid;
        assert(_ioType == FDS_IO_WRITE);
        FDS_IOType::io_type = _ioType;
        io_req_id           = _ioReqId;
        this->putObjReq = putObjReq;
        getObjReq = NULL;
        delObjReq = NULL;

        // perf-trace related data
        perfNameStr = "volume:" + std::to_string(_volUuid);
        opReqFailedPerfEventType = PUT_OBJ_REQ_ERR;

        opReqLatencyCtx.type = PUT_OBJ_REQ;
        opReqLatencyCtx.name = perfNameStr;

        opLatencyCtx.type = PUT_IO;
        opLatencyCtx.name = perfNameStr;

        opTransactionWaitCtx.type = PUT_TRANS_QUEUE_WAIT;
        opTransactionWaitCtx.name = perfNameStr;

        opQoSWaitCtx.type = PUT_QOS_QUEUE_WAIT;
        opQoSWaitCtx.name = perfNameStr;
    }

    /*
     * This constructor is generally used for
     * read, it takes a getObjReq Ptr.
     */
    // TODO(Rao): Deprecate this constructor
    SmIoReq(const std:: string& objID,
            // const std::string& _dataStr,
            FDSP_GetObjTypePtr& getObjReq,
            fds_volid_t        _volUuid,
            fds_io_op_t        _ioType,
            fds_uint32_t       _ioReqId) {
        objId = ObjectID(objID);
        // memcpy(objId.digest, objID.digest, objId.GetLen());
        // objData.size        = _dataStr.size();
        // objData.data        = _dataStr;
        volUuid             = _volUuid;
        io_vol_id           = volUuid;
        assert(_ioType == FDS_IO_READ);
        FDS_IOType::io_type = _ioType;
        io_req_id           = _ioReqId;
        this->getObjReq = getObjReq;
        putObjReq = NULL;
        delObjReq = NULL;

        // perf-trace related data
        perfNameStr = "volume:" + std::to_string(_volUuid);
        opReqFailedPerfEventType = GET_OBJ_REQ_ERR;

        opReqLatencyCtx.type = GET_OBJ_REQ;
        opReqLatencyCtx.name = perfNameStr;

        opLatencyCtx.type = GET_IO;
        opLatencyCtx.name = perfNameStr;

        opTransactionWaitCtx.type = GET_TRANS_QUEUE_WAIT;
        opTransactionWaitCtx.name = perfNameStr;

        opQoSWaitCtx.type = GET_QOS_QUEUE_WAIT;
        opQoSWaitCtx.name = perfNameStr;
    }

    SmIoReq() {
    }

    virtual ~SmIoReq() {
    }

    /*
     * This constructor is generally used for
     * write since it accepts a deleteObjReq Ptr.
     */
    // TODO(Rao): Deprecate this constructor
    SmIoReq(const std::string& objID,
            // const std::string& _dataStr,
            FDSP_DeleteObjTypePtr& delObjReq,
            fds_volid_t        _volUuid,
            fds_io_op_t        _ioType,
            fds_uint32_t       _ioReqId) {
        objId = ObjectID(objID);
        // memcpy(objId.digest, objID.digest, objId.GetLen());
        // objData.size        = _dataStr.size();
        // objData.data        = _dataStr;
        volUuid             = _volUuid;
        io_vol_id           = volUuid;
        FDS_IOType::io_type = _ioType;
        io_req_id           = _ioReqId;
        this->delObjReq = delObjReq;
        getObjReq = NULL;
        putObjReq = NULL;
    }
    void setObjId(const ObjectID &id) {
        objId = id;
    }
    const ObjectID& getObjId() const {
        return objId;
    }

    const FDSP_PutObjTypePtr&  getPutObjReq() const {
        assert(FDS_IOType::io_type == FDS_IO_WRITE);
        return putObjReq;
    }

    const FDSP_GetObjTypePtr&  getGetObjReq() const {
        return getObjReq;
    }

    const FDSP_DeleteObjTypePtr&  getDeleteObjReq() const {
        return delObjReq;
    }

    fds_volid_t getVolId() const {
        return volUuid;
    }
    void setVolId(const fds_volid_t& id) {
        volUuid = id;
        io_vol_id = id;
    }

    TransJournalId&  getTransId() {
        return transId;
    }

    void setTransId(TransJournalId trans_id) {
        transId = trans_id;
    }

    virtual std::string log_string() {
        // TODO(Rao): Fill it up
        std::stringstream ret;
        return ret.str();
    }
};

/**
 * Handlers that process SmIoReq should derive from this
 */
class SmIoReqHandler {
 public:
    virtual Error enqueueMsg(fds_volid_t volId, SmIoReq* ioReq) = 0;
};

class SmIoPutObjectReq : public SmIoReq {
 public:
    typedef std::function<void (const Error&, SmIoPutObjectReq *resp)> CbType;
 public:
    virtual std::string log_string() override;

    /* Client assigned timestamp */
    int64_t origin_timestamp;
    /* Data */
    std::string data_obj;

    /* Response callback */
    CbType response_cb;
};

typedef boost::shared_ptr<FDSP_MigrateObjectList> FDSP_MigrateObjectListPtr;
/**
 * @brief Putting token objects request
 */
class SmIoPutTokObjectsReq : public SmIoReq {
 public:
    typedef std::function<void (const Error&, SmIoPutTokObjectsReq *resp)> CbType;
 public:
    virtual std::string log_string() override
    {
        std::stringstream ret;
        ret << " SmIoPutTokObjectsReq";
        return ret.str();
    }

    /* In: Token id that objects belong to */
    fds_token_id token_id;
    /* List objects and their metadata */
    FDSP_MigrateObjectList obj_list;
    /* Response callback */
    CbType response_cb;
};

/**
 * Token iterator
 * TODO(Rao): Provide the following implementations
 * begin(), end(), next() similar to iterators.
 */
class SMTokenItr {
 public:
    leveldb::Iterator* itr;
    leveldb::DB* db;
    leveldb::ReadOptions options;
    bool done;

    SMTokenItr() {
        itr = nullptr;
        db = nullptr;
        done = false;
    }
    ~SMTokenItr() {
    }

    bool isEnd() {return done;}
};

/**
 * @brief Getting token objects request
 */
class SmIoGetTokObjectsReq : public SmIoReq {
 public:
    typedef std::function<void (const Error&, SmIoGetTokObjectsReq *resp)> CbType;
 public:
    virtual std::string log_string() override
    {
        std::stringstream ret;
        ret << " SmIoGetTokObjectsReq Token id: " << token_id;
        return ret.str();
    }

    /* In: Token id that objects belong to */
    fds_token_id token_id;
    /* Out: List objects and their metadata */
    FDSP_MigrateObjectList obj_list;
    /* In/Out: Iterator to keep track of where we were */
    SMTokenItr itr;
    /* In: Maximum size to populate */
    size_t max_size;
    /* Response callback */
    CbType response_cb;
};
typedef boost::shared_ptr<SmIoGetTokObjectsReq> SmIoGetTokObjectReqSPtr;
typedef std::unique_ptr<SmIoGetTokObjectsReq> SmIoGetTokObjectsReqUPtr;

/**
 * @brief Takes snapshot of object db
 */
class SmIoSnapshotObjectDB : public SmIoReq {
 public:
    typedef std::function<void (const Error&,
                                SmIoSnapshotObjectDB*,
                                leveldb::ReadOptions& options,
                                leveldb::DB* db)> CbType;
 public:
    SmIoSnapshotObjectDB() {
        token_id = 0;
    }

    /* In: Token to take snapshot of*/
    fds_token_id token_id;
    /* Response callback */
    CbType smio_snap_resp_cb;
};

/**
 * @brief Applies meta data transferred as part of sync
 */
class SmIoApplySyncMetadata : public SmIoReq {
 public:
    typedef std::function<void (const Error&,
                                SmIoApplySyncMetadata *sync_md)> CbType;
 public:
    SmIoApplySyncMetadata() {
    }
    virtual std::string log_string() override
    {
        std::stringstream ret;
        ret << " SmIoApplySyncMetadata object id: " << md.object_id.digest;
        return ret.str();
    }

    /* In: Sync metadata list */
    FDSP_MigrateObjectMetadata md;
    /* Out: data physically exists for this md or not */
    bool dataExists;
    /* Response callback */
    CbType smio_sync_md_resp_cb;
};

/**
 * @brief Applies meta data transferred as part of sync
 */
class SmIoResolveSyncEntry : public SmIoReq {
 public:
    typedef std::function<void (const Error&, SmIoResolveSyncEntry*)> CbType;
 public:
    SmIoResolveSyncEntry() {
    }

    /* In: Sync metadata list */
    ObjectID object_id;
    /* Response callback */
    CbType smio_resolve_resp_cb;
};

/**
 * @brief Applies object data
 */
class SmIoApplyObjectdata : public SmIoReq {
 public:
    typedef std::function<void (const Error&,
                                SmIoApplyObjectdata *sync_md)> CbType;
 public:
    SmIoApplyObjectdata() {
    }

    /* In: object id */
    ObjectID obj_id;
    /* In: object_data */
    std::string obj_data;

    /* Response callback */
    CbType smio_apply_data_resp_cb;
};

/**
 * @brief For reading object data
 */
class SmIoReadObjectdata : public SmIoReq {
 public:
    typedef std::function<void (const Error&,
                                SmIoReadObjectdata *read_data)> CbType;
 public:
    SmIoReadObjectdata() {
    }
    /* In/out: In is object id, out is object data */
    FDSP_ObjectIdDataPair obj_data;

    /* Response callback */
    CbType smio_readdata_resp_cb;
};

/**
 * @brief For reading object metadata
 */
class SmIoReadObjectMetadata : public SmIoReq {
 public:
    typedef std::function<void (const Error&,
                                SmIoReadObjectMetadata *read_data)> CbType;
 public:
    SmIoReadObjectMetadata() {
    }
    /* In/out: In is object id, out is object meta data */
    FDSP_MigrateObjectMetadata meta_data;
    /* In: Cookie from the caller for caching any context info */
    boost::shared_ptr<void> cookie;

    /* Response callback */
    CbType smio_readmd_resp_cb;
};

/**
 * Request to copy or delete a given list of objects
 */
class SmIoCompactObjects : public SmIoReq {
 public:
    typedef std::function<void (const Error&,
                                SmIoCompactObjects *req)> cbType;
 public:
    SmIoCompactObjects() {}

    /* list of object ids */
    std::vector<ObjectID> oid_list;

    /* tier that we are compacting */
    diskio::DataTier tier;

    /* response callback */
    cbType smio_compactobj_resp_cb;
};
}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_SMIO_H_
