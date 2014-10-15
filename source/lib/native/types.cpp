/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <native/types.h>
#include <string>

namespace fds {
FdsBlobReq::FdsBlobReq(fds_io_op_t      _op,
                       fds_volid_t        _volId,
                       const std::string &_blobName,
                       fds_uint64_t       _blobOffset,
                       fds_uint64_t       _dataLen,
                       char              *_dataBuf)
        : magic(FDS_SH_IO_MAGIC_IN_USE),
          ioType(_op),
          volId(_volId),
          blobName(_blobName),
          blobOffset(_blobOffset),
          dataLen(_dataLen),
          dataBuf(_dataBuf) {
}

FdsBlobReq::FdsBlobReq(fds_io_op_t       _op,
                       fds_volid_t        _volId,
                       const std::string &_blobName,
                       fds_uint64_t       _blobOffset,
                       fds_uint64_t       _dataLen,
                       char              *_dataBuf,
                       CallbackPtr        _cb)
        : magic(FDS_SH_IO_MAGIC_IN_USE),
          ioType(_op),
          volId(_volId),
          blobName(_blobName),
          blobOffset(_blobOffset),
          dataLen(_dataLen),
          dataBuf(_dataBuf),
          cb(_cb) {
}

FdsBlobReq::~FdsBlobReq() {
    magic = FDS_SH_IO_MAGIC_NOT_IN_USE;
}
fds_bool_t FdsBlobReq::magicInUse() const {
    return (magic == FDS_SH_IO_MAGIC_IN_USE);
}

fds_volid_t FdsBlobReq::getVolId() const {
    return volId;
}

fds_io_op_t  FdsBlobReq::getIoType() const {
    return ioType;
}

void FdsBlobReq::setVolId(fds_volid_t vol_id) {
    volId = vol_id;
}

void FdsBlobReq::cbWithResult(int result) {
    return callback(result);
}

const std::string& FdsBlobReq::getBlobName() const {
    return blobName;
}

const std::string& FdsBlobReq::getVolumeName() const {
    return volumeName;
}

void FdsBlobReq::setVolumeName(const std::string& volumeName) {
    this->volumeName = volumeName;
}

fds_uint64_t FdsBlobReq::getBlobOffset() const {
    return blobOffset;
}

void FdsBlobReq::setBlobOffset(fds_uint64_t offset) {
    blobOffset = offset;
}

const char* FdsBlobReq::getDataBuf() const {
    return dataBuf;
}

std::size_t FdsBlobReq::getDataLen() const {
    return dataLen;
}

void FdsBlobReq::setDataLen(fds_uint64_t len) {
    dataLen = len;
}

void FdsBlobReq::setDataBuf(const char* _buf) {
    /*
     * TODO: For now we're assuming the buffer is preallocated
     * by the owner and the length has been set already.
     */
    memcpy(dataBuf, _buf, dataLen);
}

ObjectID FdsBlobReq::getObjId() const
{
    return objId;
}

void FdsBlobReq::setObjId(const ObjectID& _oid) {
    objId = _oid;
}

void FdsBlobReq::setQueuedUsec(fds_uint64_t _usec) {
    queuedUsec = _usec;
}

BucketContext::BucketContext(const std::string& _hostName,
                             const std::string& _bucketName,
                             const std::string& _accessKeyId,
                             const std::string& _secretAccessKey)
        : hostName(_hostName),
          bucketName(_bucketName),
          accessKeyId(_accessKeyId),
          secretAccessKey(_secretAccessKey) {
}

BucketContext::~BucketContext() {
}

QosParams::QosParams(double _iops_min,
                     double _iops_max,
                     int _prio)
        : iops_min(_iops_min),
          iops_max(_iops_max),
          relativePrio(_prio) {
}

QosParams::~QosParams() {
}

ListBucketContents::ListBucketContents() {}
ListBucketContents::~ListBucketContents() {}

void ListBucketContents::set(const std::string& _key,
                             fds_uint64_t _modified,
                             const std::string& _eTag,
                             fds_uint64_t _size,
                             const std::string& _ownerId,
                             const std::string& _ownerDisplayName) {
    objKey = _key;
    lastModified = _modified;
    eTag = _eTag;
    size = _size;
    ownerId = _ownerId;
    ownerDisplayName = _ownerDisplayName;
}

BucketStatsContent::BucketStatsContent() : bucket_name(""),
                                           priority(0),
                                           performance(0),
                                           sla(0),
                                           limit(0) {
}

BucketStatsContent::~BucketStatsContent() {
}

void BucketStatsContent::set(const std::string& _name,
                             int _prio,
                             double _perf,
                             double _sla,
                             double _limit) {
    bucket_name = _name;
    priority = _prio;

    /* we are currently returning values from 0 to 100,
     * so normalize the values */
    performance = _perf / static_cast<double>(FDSN_QOS_PERF_NORMALIZER);
    if (performance > 100.0) {
        performance = 100.0;
    }
    sla = _sla / static_cast<double>(FDSN_QOS_PERF_NORMALIZER);
    if (sla > 100.0) {
        sla = 100.0;
    }
    limit = _limit / static_cast<double>(FDSN_QOS_PERF_NORMALIZER);
    if (limit > 100.0) {
        limit = 100.0;
    }
}

Callback::~Callback() {
    if (errorDetails) delete errorDetails;
}

void Callback::operator()(FDSN_Status status) {
    call(status);
}

void Callback::call(FDSN_Status status) {
    this->status = status;
    this->error  = status;
    call();
}

void Callback::call(Error err) {
    this->error = err;
    this->status = err.GetErrno();
    call();
}

bool Callback::isStatusSet() {
    return status != FDSN_StatusErrorUnknown;
}

bool Callback::isErrorSet() {
    return error != ERR_MAX;
}

ScopedCallBack::ScopedCallBack(CallbackPtr cb) : cb(cb) {
}

ScopedCallBack::~ScopedCallBack() {
    cb->call();
}

}  // namespace fds
