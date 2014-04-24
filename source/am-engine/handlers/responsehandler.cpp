/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <am-engine/handlers/responsehandler.h>
#include <util/Log.h>
#include <util/timeutils.h>
#include <string>
#include <sstream>

#define XCHECKSTATUS(status)             \
    if (status != FDSN_StatusOK) {       \
        LOGWARN << " status:" << status; \
        xdi::XdiException xe;            \
        std::ostringstream oss;          \
        oss << status;                   \
        xe.errorCode = xdi::ErrorCode::INTERNAL_SERVER_ERROR;   \
        xe.message = oss.str();          \
        throw xe;                        \
    }

namespace fds {

void ResponseHandler::call() {
    switch (type) {
        case HandlerType::WAITEDFOR:
            ready(); break;
        case HandlerType::IMMEDIATE:
            process() ; break;
        case HandlerType::QUEUED:
            fds_panic("QUEUED - not implemented yet");
            break;
    }
}

void ResponseHandler::ready() {
    task.done();
}

void ResponseHandler::wait() {
    task.await();
}

bool ResponseHandler::waitFor(ulong timeout) {
    return task.await(timeout);
}

ResponseHandler::~ResponseHandler() {
}

//================================================================================

SimpleResponseHandler::SimpleResponseHandler() {
}

SimpleResponseHandler::SimpleResponseHandler(const std::string& name) : name(name) {
}

void SimpleResponseHandler::process() {
    LOGDEBUG << "processing callback for : " << name;
    XCHECKSTATUS(status);
}

SimpleResponseHandler::~SimpleResponseHandler() {
}

//================================================================================

void PutObjectResponseHandler::process() {
}

PutObjectResponseHandler::~PutObjectResponseHandler() {
}
//================================================================================

void GetObjectResponseHandler::process() {
}

GetObjectResponseHandler::~GetObjectResponseHandler() {
}
//================================================================================

void ListBucketResponseHandler::process() {
}

ListBucketResponseHandler::~ListBucketResponseHandler() {
}

//================================================================================

BucketStatsResponseHandler::BucketStatsResponseHandler(
    xdi::VolumeDescriptor& volumeDescriptor) : volumeDescriptor(volumeDescriptor) {
}

void BucketStatsResponseHandler::process() {
    XCHECKSTATUS(status);
    volumeDescriptor.name = contents->bucket_name;
    // volumeDescriptor.uuid = 10;
    volumeDescriptor.dateCreated = util::getTimeStampMillis();
    volumeDescriptor.policy.objectSizeInBytes = 2097152;  // 2MB
}

BucketStatsResponseHandler::~BucketStatsResponseHandler() {
    if (contents) {
        delete contents;
    }
}
}  // namespace fds
