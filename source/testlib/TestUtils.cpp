/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <fdsp/svc_types_types.h>
#include <boost/filesystem.hpp>
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

bool findFdsSrcPath(std::string &fdsSrcPath)
{
    namespace bfs = boost::filesystem;
    bfs::path p(boost::filesystem::current_path());

    while (!bfs::is_empty(p)) {
        if (p.filename().string() == "source" &&
            !bfs::is_empty(p.parent_path()) &&
            p.parent_path().filename().string() == "fds-src") {
            fdsSrcPath = p.string();
            return true;
        }
        p = p.parent_path();
    }
    return false;
}

}  // namespace TestUtils
}  // namespace fds
