/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <am-engine/handlers/responsehandler.h>
#include <xdi/am_shim_types.h>
#include <util/Log.h>
#include <string>

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
    if (status != FDSN_StatusOK) {
        LOGWARN << " handler:" << name
                << " status:" << status;
        throw xdi::XdiException();
    }
}

SimpleResponseHandler::~SimpleResponseHandler() {
}

//================================================================================

void PutObjectResponseHandler::process() {
}

PutObjectResponseHandler::~PutObjectResponseHandler(){
}
//================================================================================

void GetObjectResponseHandler::process() {
}

GetObjectResponseHandler::~GetObjectResponseHandler(){
}
//================================================================================

void ListBucketResponseHandler::process() {
}

ListBucketResponseHandler::~ListBucketResponseHandler() {
}

//================================================================================

void BucketStatsResponseHandler::process() {
}

BucketStatsResponseHandler::~BucketStatsResponseHandler() {
}
}  // namespace fds
