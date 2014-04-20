/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <am-engine/handlers/responsehandler.h>

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

void SimpleResponseHandler::process() {
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
