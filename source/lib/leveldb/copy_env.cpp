/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <leveldb/status.h>
#include <leveldb/copy_env.h>
#include <db/filename.h>

/**
 * XXX: This code is copied (with a few changes) from the leveldb forum post:
 * https://code.google.com/p/leveldb/issues/detail?id=184
 *
 * The solution best documented on the mentioned web-page.
 */

namespace leveldb {

Status CopyEnv::DeleteFile(const std::string & f) {
    FDSGUARD(mtx_);
    Status s;
    if (copying_) {
        deferredDeletions_.push_back(f);
    } else {
        s = target()->DeleteFile(f);
    }

    return s;
}

// Call (*save)(arg, filename, length) for every file that
// should be copied to construct a consistent view of the
// database. "length" may be negative to indicate that the entire
// file must be copied. Otherwise the first "length" bytes must be
// copied
Status CopyEnv::Copy(const std::string & dir, int(*save)(void *, const char* fname,
        fds_uint64_t length), void * arg) {
    // Get consistent view of all files.
    mtx_.lock();

    std::vector<std::string> files;
    Status s = GetChildren(dir, &files);
    if (!s.ok()) {
        mtx_.unlock();
        return s;
    }

    std::vector<fds_uint64_t> lengths(files.size());
    for (size_t i = 0; i < files.size(); i++) {
        if ('.' == files[i][0]) {
            continue;
        }

        if (CopyFile(files[i])) {
            uint64_t len;
            s = GetFileSize(dir + "/" + files[i], &len);
            if (!s.ok()) {
                mtx_.unlock();
                return s;
            }
            lengths[i] = len;
        } else {
            lengths[i] = -1;
        }
    }

    copying_ = true;
    mtx_.unlock();

    for (size_t i = 0; s.ok() && i < files.size(); ++i) {
        if (KeepFile(files[i])) {
            if ((*save)(arg, files[i].c_str(), lengths[i]) != 0) {
                s = Status::IOError("copy failed");
                break;
            }
        }
    }

    {
        FDSGUARD(mtx_);
        copying_ = false;
        for (size_t i = 0; i < deferredDeletions_.size(); ++i) {
            target()->DeleteFile(deferredDeletions_[i]);
        }
        deferredDeletions_.clear();
    }

    return s;
}

fds_bool_t CopyEnv::CopyFile(const std::string& fname) {
    uint64_t number;
    FileType type;
    ParseFileName(fname, &number, &type);
    return type != kTableFile;
}

fds_bool_t CopyEnv::KeepFile(const std::string & fname) {
    uint64_t number;
    FileType type;

    if (ParseFileName(fname, &number, &type)) {
        switch (type) {
        case kLogFile:
        case kTableFile:
        case kDescriptorFile:
        case kCurrentFile:
        case kInfoLogFile:
            return true;
        case kDBLockFile:
        case kTempFile:
            return false;
        }
    }

    return false;
}

} // namespace leveldb
