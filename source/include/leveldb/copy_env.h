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

const std::string DEFAULT_ARCHIVE_PREFIX("catalog.archive");
const fds_uint32_t MAX_NUM_LOGFILES = 10;

class CopyEnv : public leveldb::EnvWrapper {
private:
    fds_mutex mtx_;
    fds_mutex rotateMtx_;
    fds_bool_t copying_;
    fds_bool_t logRotate_;
    fds_bool_t archiveLogs_;

    std::string logDirName_;
    std::string logFilePrefix_;
    std::string archivePrefix_;

    fds_uint32_t maxLogFiles_;

    std::vector<std::string> deferredDeletions_;

public:
    explicit CopyEnv(leveldb::Env& t) : leveldb::EnvWrapper(&t),
                                        copying_(false),
                                        logRotate_(false),
                                        logFilePrefix_("catalog.journal"),
                                        archivePrefix_(DEFAULT_ARCHIVE_PREFIX),
                                        maxLogFiles_(0)
    {}

    Status NewWritableFile(const std::string& fname, WritableFile** result) override;

    Status DeleteFile(const std::string & f) override;

    Status DeleteDir(const std::string & dir) override;

    // Call (*save)(arg, filename, length) for every file that
    // should be copied to construct a consistent view of the
    // database. "length" may be negative to indicate that the entire
    // file must be copied. Otherwise the first "length" bytes must be
    // copied
    Status Copy(const std::string & dir, int(*save)(void *, const char* fname,
            fds_uint64_t length), void * arg);

    fds_bool_t CopyFile(const std::string & fname);
    fds_bool_t KeepFile(const std::string & fname);

    inline const fds_bool_t & archiveLogs() const {
        return archiveLogs_;
    }
    inline fds_bool_t & archiveLogs() {
        return archiveLogs_;
    }

    inline const fds_bool_t & logRotate() const {
        return logRotate_;
    }
    inline fds_bool_t & logRotate() {
        return logRotate_;
    }

    inline const std::string & logDirName() const {
        return logDirName_;
    }
    inline std::string & logDirName() {
        return  logDirName_;
    }

    inline const std::string & logFilePrefix() const {
        return logFilePrefix_;
    }
    inline std::string & logFilePrefix() {
        return logFilePrefix_;
    }

    inline const std::string & archivePrefix() const {
        return archivePrefix_;
    }
    inline std::string & archivePrefix() {
        return archivePrefix_;
    }

    inline const fds_uint32_t & maxLogFiles() const {
        return maxLogFiles_;
    }
    inline fds_uint32_t & maxLogFiles() {
        return maxLogFiles_;
    }
};

} // namespace leveldb
#endif // SOURCE_INCLUDE_LEVELDB_COPY_ENV_H_
