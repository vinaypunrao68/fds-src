/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <am-engine/handlers/responsehandler.h>
#include <util/Log.h>

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

SimpleResponseHandler::SimpleResponseHandler() {
}

SimpleResponseHandler::SimpleResponseHandler(const std::string& name) : name(name) {
}

void SimpleResponseHandler::process() {
    LOGDEBUG << "processing callback for : " << name;
    if (status != FDSN_StatusOK) {
        LOGWARN << " handler:" << name
                << " status:" << status ;
        throw xdi::XdiException();
    }
}

void PutObjectResponseHandler::process() {
}

void GetObjectResponseHandler::process() {
}

void ListBucketResponseHandler::process() {
}

void BucketStatsResponseHandler::process() {
}

}  // namespace fds
