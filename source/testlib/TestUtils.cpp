/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <fdsp/svc_types_types.h>
#include <TestUtils.h>

namespace fds { namespace TestUtils {

void Waiter::doneWith(const Error &e) {
    if (error == ERR_OK) {
        error = e;
    }
    done();
}

void Waiter::reset(uint32_t numTasks) {
    error = ERR_OK;
    concurrency::TaskStatus::reset(numTasks);
}

Error Waiter::awaitResult() {
    await();
    return error;
}

Error Waiter::awaitResult(ulong timeout) {
    await(timeout);
    return error;
}

}  // namespace TestUtils
}  // namespace fds
