/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#ifndef SOURCE_STOR_MGR_INCLUDE_SMIO_H_
#define SOURCE_STOR_MGR_INCLUDE_SMIO_H_

#include <list>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <boost/shared_ptr.hpp>

#include <fdsp/sm_api_types.h>
#include <fds_error.h>
#include <fds_types.h>
#include <fds_volume.h>
#include <leveldb/db.h>
#include <leveldb/copy_env.h>
#include <persistent-layer/dm_io.h>
#include <SmTypes.h>
#include <ObjMeta.h>

using FDS_ProtocolInterface::FDSP_DeleteObjTypePtr;
using FDS_ProtocolInterface::FDSP_GetObjTypePtr;
using FDS_ProtocolInterface::FDSP_ObjectIdDataPair;
using FDS_ProtocolInterface::FDSP_PutObjTypePtr;

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
    unsigned int   transId;
    FDSP_PutObjTypePtr putObjReq;
    FDSP_GetObjTypePtr getObjReq;
    FDSP_DeleteObjTypePtr delObjReq;

    /// Client service ID used for serialization.
    /// The Thrift generated constructor automatically
    /// defaults the value to 0.
    fpi::SvcUuid clientSvcId;

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
        opReqFailedPerfEventType = PerfEventType::SM_PUT_OBJ_REQ_ERR;  // FIXME(matteo): what is this?

        opReqLatencyCtx.type = PerfEventType::SM_E2E_PUT_OBJ_REQ;
        opReqLatencyCtx.reset_volid(_volUuid);

        opLatencyCtx.type = PerfEventType::SM_PUT_IO;
        opLatencyCtx.reset_volid(_volUuid);

        opQoSWaitCtx.type = PerfEventType::SM_PUT_QOS_QUEUE_WAIT;
        opQoSWaitCtx.reset_volid(_volUuid);
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
        opReqFailedPerfEventType = PerfEventType::SM_GET_OBJ_REQ_ERR;  // FIXME(matteo): what is this?

        opReqLatencyCtx.type = PerfEventType::SM_E2E_GET_OBJ_REQ;
        opReqLatencyCtx.reset_volid(_volUuid);

        opLatencyCtx.type = PerfEventType::SM_GET_IO;
        opLatencyCtx.reset_volid(_volUuid);

        opQoSWaitCtx.type = PerfEventType::SM_GET_QOS_QUEUE_WAIT;
        opQoSWaitCtx.reset_volid(_volUuid);
    }

    SmIoReq() {
    }

    virtual ~SmIoReq() {
    }

    /*
     * This constructor is generally used for
     * delete since it accepts a deleteObjReq Ptr.
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

        // perf-trace related data
        opReqFailedPerfEventType = PerfEventType::SM_DELETE_OBJ_REQ_ERR;  // FIXME(matteo): what is this?

        opReqLatencyCtx.type = PerfEventType::SM_E2E_DELETE_OBJ_REQ;
        opReqLatencyCtx.reset_volid(_volUuid);

        opLatencyCtx.type = PerfEventType::SM_DELETE_IO;
        opLatencyCtx.reset_volid(_volUuid);

        opQoSWaitCtx.type = PerfEventType::SM_DELETE_QOS_QUEUE_WAIT;
        opQoSWaitCtx.reset_volid(_volUuid);
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

    unsigned int&  getTransId() {
        return transId;
    }

    void setTransId(unsigned int trans_id) {
        transId = trans_id;
    }

    void setClientSvcId(const fpi::SvcUuid& id) {
        clientSvcId = id;
    }

    fpi::SvcUuid getClientSvcId() const {
        return clientSvcId;
    }

    virtual std::string log_string() {
        // TODO(Rao): Fill it up
        std::stringstream ret;
        return ret.str();
    }
};  // class SmIoReq

/**
 * Handlers that process SmIoReq should derive from this
 */
class SmIoReqHandler {
 public:
    virtual Error enqueueMsg(fds_volid_t volId, SmIoReq* ioReq) = 0;
};  // class SmIoReqHandler

class SmIoAddObjRefReq : public SmIoReq {
  public:
    typedef std::function<void (const Error&, SmIoAddObjRefReq * resp)> CbType;
    virtual std::string log_string() override;

    // ctor and dtor
    explicit SmIoAddObjRefReq(fpi::AddObjectRefMsgPtr addObjRefReq_)
            : addObjRefReq(addObjRefReq_),
              forwardedReq(false)
    {
        fds_assert(NULL != addObjRefReq.get());

        io_type = FDS_SM_ADD_OBJECT_REF;
        volUuid = addObjRefReq->srcVolId;
        io_vol_id = addObjRefReq->srcVolId;
    }

    fds_volid_t getSrcVolId() {
        return fds_volid_t(addObjRefReq->srcVolId);
    }

    fds_volid_t getDestVolId() {
        return fds_volid_t(addObjRefReq->destVolId);
    }

    std::vector<fpi::FDS_ObjectIdType> & objIds() {
        return addObjRefReq->objIds;
    }

    virtual ~SmIoAddObjRefReq() {}

    // member variables
    fpi::AddObjectRefMsgPtr addObjRefReq;
    CbType response_cb;

    /// DLT version for the AddObjRef request
    fds_uint64_t dltVersion;

    /// If the AddObjRef requestes was forwarded by the SM token migration
    bool forwardedReq;
};  // class SmIoAddObjRefReq

/**
 * @brief For DEL object data
 */
class SmIoDeleteObjectReq : public SmIoReq {
  public:
    typedef std::function<void (const Error&, SmIoDeleteObjectReq *resp)> CbType;
    virtual std::string log_string() override;

    explicit SmIoDeleteObjectReq(boost::shared_ptr<fpi::DeleteObjectMsg> &msg)
        : delObjectNetReq(msg),
          forwardedReq(false) {
    }

    /// Service layer put request
    boost::shared_ptr<fpi::DeleteObjectMsg> delObjectNetReq;

    /// Callback after completion of DELETE object request.
    CbType response_cb;

    /// DLT version for the DELETE request
    fds_uint64_t dltVersion;

    /// If the DELETE request was forwarded by the SM token migration.
    bool forwardedReq;

    friend std::ostream& operator<< (std::ostream &out,
                                     const SmIoDeleteObjectReq& delReq) {
        out << "DELETE object request:" << delReq.getObjId()
            << " volume=" << std::hex << delReq.getVolId() << std::dec
            << " DLTversion=" << delReq.dltVersion
            << " forwarded=" << delReq.forwardedReq;
        return out;
    }
};  // class SmIoDeleteObjectReq

/**
 * @brief For PUT object data
 */
class SmIoPutObjectReq : public SmIoReq {
 public:
    typedef std::function<void (const Error&, SmIoPutObjectReq *resp)> CbType;
    virtual std::string log_string() override;

    explicit SmIoPutObjectReq(boost::shared_ptr<fpi::PutObjectMsg>& msg)
            : putObjectNetReq(msg),
              forwardedReq(false) {
    }

    /// Service layer put request
    boost::shared_ptr<fpi::PutObjectMsg> putObjectNetReq;

    /// Response callback after completion of PUT object request
    CbType response_cb;

    /// DLT version for the PUT request
    fds_uint64_t dltVersion;

    /// if the PUT request was forwarded by the SM token migration
    bool forwardedReq;

    friend std::ostream& operator<< (std::ostream &out,
                                     const SmIoPutObjectReq& putReq) {
        out << "PUT object request: " << putReq.getObjId()
            << " volume=" << std::hex << putReq.getVolId() << std::dec
            << " DLTversion=" << putReq.dltVersion
            << " forwarded=" << putReq.forwardedReq;
        return out;
    }
};  // class SmIoPutObjectReq

/**
 * @brief For GET object data
 */
class SmIoGetObjectReq : public SmIoReq {
 public:
    typedef std::function<void (const Error&, SmIoGetObjectReq *resp)> CbType;
    virtual std::string log_string() override;

    explicit SmIoGetObjectReq(boost::shared_ptr<fpi::GetObjectMsg> &msg)
            : getObjectNetReq(msg) {
        getObjectNetResp = boost::make_shared<fpi::GetObjectResp>();
    }

    /* In/out: In is object id, out is object data */
    FDSP_ObjectIdDataPair obj_data;

    /// Service layer get request
    boost::shared_ptr<fpi::GetObjectMsg> getObjectNetReq;
    /// Service layer get response
    boost::shared_ptr<fpi::GetObjectResp> getObjectNetResp;

    /// Response callback
    CbType response_cb;
};  // class SmIoGetObjectReq

/**
 * Token iterator
 * TODO(Rao): Provide the following implementations
 * begin(), end(), next() similar to iterators.
 */
class SMTokenItr {
 public:
    leveldb::Iterator* itr;
    std::shared_ptr<leveldb::DB> db;
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
};   // class SmTokenIter

/**
 * @brief Takes snapshot of object db
 */
class SmIoSnapshotObjectDB : public SmIoReq {
 public:
    typedef std::function<void (const Error&,
                                SmIoSnapshotObjectDB*,
                                leveldb::ReadOptions& options,
                                std::shared_ptr<leveldb::DB> db,
                                bool retry,
                                fds_uint32_t uid)> CbType;
    typedef std::function<void (const Error&,
                                SmIoSnapshotObjectDB*,
                                std::string &snapDir,
                                leveldb::CopyEnv *env)> CbTypePersist;
 public:
    SmIoSnapshotObjectDB() {
        token_id = 0;
        unique_id = 0;
        isPersistent = false;
        executorId = SM_INVALID_EXECUTOR_ID;
        snapNum = "";
        targetDltVersion = 0;
    }

    /* In: Token to take snapshot of*/
    fds_token_id token_id;

    /* In: Unique id passed to be checked when
     * handing over snapshot to migration executors.
     */
     fds_uint32_t unique_id;

    /* In: Persistant or in-memory snapshot?
     */
    bool isPersistent;

    /**
     * ID of the executor for which this snapshort is taken for
     * 0 if we are taking snapshot not for token migration
     */
    fds_uint64_t executorId;

    /**
     * SM token's snapshot number
     */
    std::string snapNum;

    /**
     * Target DLT version for which this snapshot will be taken.
     */
    fds_uint64_t targetDltVersion;

    /**
     * Set if snapshot request is for retry of a SM token migration
     */
    bool retryReq;

    /* Response callback for in-memory snapshot request*/
    CbType smio_snap_resp_cb;

    /* Response callback for persistent snapshot request */
    CbTypePersist smio_persist_snap_resp_cb;

    friend std::ostream& operator<< (std::ostream &out,
                                     const SmIoSnapshotObjectDB& snapReq) {
        out << "SmIoSnapshotObjectDB: SM token " << snapReq.token_id
            << " executorId " << snapReq.executorId;
        return out;
    }
};  // class SmIoSnapshotObjectDB

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

    /// also verify data before compacting
    fds_bool_t verifyData;

    /* response callback */
    cbType smio_compactobj_resp_cb;
};  // class SmIoCompactObjects

/**
 * Request to move a set of objects from one tier to another tier
 */
class SmIoMoveObjsToTier: public SmIoReq {
  public:
    typedef std::function<void (const Error&,
                                SmIoMoveObjsToTier *req)> cbType;
  public:
    SmIoMoveObjsToTier() {
        moveObjsRespCb = NULL;
        relocate = false;
        movedCnt = 0;
    }

    /// list of object ids
    std::vector<ObjectID> oidList;

    /// tier to move from
    diskio::DataTier fromTier;

    /// tier to move to
    diskio::DataTier toTier;

    /// if true, relocate objects (remove from fromTier)
    fds_bool_t relocate;

    /// moved cnt
    uint32_t movedCnt;

    /// response callback
    cbType moveObjsRespCb;
};  // class SmIoMoveObjsToTier

/**
 * Request to read delta set from set of meta data.
 */
class SmIoReadObjDeltaSetReq: public SmIoReq {
  public:
    typedef std::function<void (const Error&,
                                SmIoReadObjDeltaSetReq *req)> cbType;
  public:
    SmIoReadObjDeltaSetReq(const NodeUuid& destSmId,
                           fds_uint64_t execId,
                           fds_uint64_t seq,
                           fds_bool_t last)
        : destinationSmId(destSmId),
          executorId(execId),
          seqNum(seq),
          lastSet(last)
    {
    };

    // Node ID of the destination SM.
    NodeUuid destinationSmId;

    // ID of the executor from the source SM, which requested the delta set.
    fds_uint64_t executorId;

    // Sequence number to indicate all messages are received on the destionation SM.
    fds_uint64_t seqNum;

    // last set of the current transaction between source and destination SM.
    fds_bool_t lastSet;

    // Set of Object MetaData to be used to read objects and form
    // CtrlObjectRebalanceDeltaSet
    // vector of a pair <ObjMetaPata::ptr, bool reconcileMetaData>
    std::vector<std::pair<ObjMetaData::ptr, fpi::ObjectMetaDataReconcileFlags>> deltaSet;

    // Response callback for batch object read.
    cbType smioReadObjDeltaSetReqCb;
};  // class SmIoReadObjDelta

typedef boost::shared_ptr<SmIoReadObjDeltaSetReq> SmIoReadObjDeltaSetReqSharedPtr;
typedef std::unique_ptr<SmIoReadObjDeltaSetReq> SmIoReadObjDeltaSetReqUniquePtr;

/**
 * Request to apply object rebalance delta set
 */
class SmIoApplyObjRebalDeltaSet: public SmIoReq {
  public:
    typedef std::function<void (const Error&,
                                SmIoApplyObjRebalDeltaSet *req)> cbType;

  public:
    SmIoApplyObjRebalDeltaSet(fds_uint64_t execId,
                              fds_uint64_t seq,
                              fds_bool_t last,
                              fds_uint64_t qosSeq,
                              fds_bool_t qosLast)
            : executorId(execId), seqNum(seq), lastSet(last),
            qosSeqNum(qosSeq), qosLastSet(qosLast) {
    };

    /// MigrationExecutor ID
    fds_uint64_t executorId;

    /// sequence number of CtrlObjectRebalanceDeltaSet msg from source SM
    fds_uint64_t seqNum;

    /// 'lastSet' value of CtrlObjectRebalanceDeltaSet msg from source SM
    fds_bool_t lastSet;

    /// we are breaking down object set from CtrlObjectRebalanceDeltaSet
    /// into one or more QoS requests with smaller delta set, we need to
    /// track when we finish those, so that when we apply the last set
    /// we notify token migration manager that we are done
    fds_uint64_t qosSeqNum;
    fds_bool_t qosLastSet;

    /// set of data/metadata to apply
    std::vector<fpi::CtrlObjectMetaDataPropagate> deltaSet;

    /// response callback
    cbType smioObjdeltaRespCb;
};  // class SmIoApplyObjRebalDeltaSet

/**
 * Request to abort migration
 *
 * We are making this a QoS request, since this operation can take a while
 * to complete, instead of handling  the close in the service layer, handle
 * it in the SM QoS layer.
 */
class SmIoAbortMigration: public SmIoReq {
  public:
    typedef std::function<void (const Error&,
                                SmIoAbortMigration *req)> cbType;

    explicit SmIoAbortMigration(fpi::CtrlNotifySMAbortMigrationPtr& abortMigrationMsg)
        : abortMigrationReqMsg(abortMigrationMsg) {
    }

    fpi::CtrlNotifySMAbortMigrationPtr abortMigrationReqMsg;

    fds_uint64_t abortMigrationDLTVersion;
    fds_uint64_t targetDLTVersion;

    cbType abortMigrationCb;
};  // class SmIoAbortMigration

/**
 * Reqest to close DLT.
 *
 * We are making this a QoS request, since this operation can take a while
 * to complete, instead of handling  the close in the service layer, handle
 * it in the SM QoS layer.
 */
class SmIoNotifyDLTClose: public SmIoReq {
  public:
    typedef std::function<void (const Error&,
                                SmIoNotifyDLTClose *req)> cbType;

    explicit SmIoNotifyDLTClose(boost::shared_ptr<fpi::CtrlNotifyDLTClose>& closeDLTMsg)
        : closeDLTReqMsg(closeDLTMsg) {
     }

    boost::shared_ptr<fpi::CtrlNotifyDLTClose> closeDLTReqMsg;

    fds_uint64_t closeDLTVersion;

    cbType closeDLTCb;
};  // class SmIoNotifyDLTClose


}  // namespace fds
#endif  // SOURCE_STOR_MGR_INCLUDE_SMIO_H_
