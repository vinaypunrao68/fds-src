/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <responsehandler.h>
#include <util/Log.h>
#include <util/timeutils.h>
#include <string>
#include <sstream>
#include <vector>

namespace fds {

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

ResponseHandler::~ResponseHandler() = default;

}  // namespace fds
