/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_LEVELDB_COPY_ENV_H_
#define SOURCE_INCLUDE_LEVELDB_COPY_ENV_H_

/**
 * XXX: This code is copied (with a few changes) from the leveldb forum post:
 * https://code.google.com/p/leveldb/issues/detail?id=184
 *
 * The solution best documented on the mentioned web-page.
 */

#include <leveldb/env.h>
#include <concurrency/Mutex.h>
#include <fds_types.h>

using namespace fds;

namespace leveldb {

class CopyEnv : public leveldb::EnvWrapper {
private:
    fds_mutex mtx_;
    fds_bool_t copying_;

    std::vector<std::string> deferredDeletions_;

public:
    explicit CopyEnv(leveldb::Env* t) : leveldb::EnvWrapper(t), copying_(false) {
    }

    Status DeleteFile(const std::string & f);

    // Call (*save)(arg, filename, length) for every file that
    // should be copied to construct a consistent view of the
    // database. "length" may be negative to indicate that the entire
    // file must be copied. Otherwise the first "length" bytes must be
    // copied
    Status Copy(const std::string & dir, int(*save)(void *, const char* fname,
            fds_uint64_t length), void * arg);

    fds_bool_t CopyFile(const std::string & fname);
    fds_bool_t KeepFile(const std::string & fname);
};

} // namespace leveldb
#endif // SOURCE_INCLUDE_LEVELDB_COPY_ENV_H_
