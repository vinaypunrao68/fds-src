/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <native/types.h>
#include <string>

namespace fds {
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
