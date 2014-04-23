/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <am-engine/handlers/responsehandler.h>
#include <util/Log.h>
#include <string>
#include <sstream>

#define XCHECKSTATUS(status)             \
    if (status != FDSN_StatusOK) {       \
        LOGWARN << " status:" << status; \
        xdi::XdiException xe;            \
        std::ostringstream oss;          \
        oss << status;                   \
        xe.errorCode = status;           \
        xe.message = oss.str();          \
        throw xe;                        \
    }

namespace fds {
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
}

BucketStatsResponseHandler::~BucketStatsResponseHandler() {
}
}  // namespace fds
