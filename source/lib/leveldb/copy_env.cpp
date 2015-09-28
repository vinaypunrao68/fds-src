/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <leveldb/status.h>
#include <leveldb/copy_env.h>
#include <db/filename.h>

#include <util/timeutils.h>

/**
 * XXX: This code is copied (with a few changes) from the leveldb forum post:
 * https://code.google.com/p/leveldb/issues/detail?id=184
 *
 * The solution best documented on the mentioned web-page.
 */

namespace leveldb {

Status CopyEnv::NewWritableFile(const std::string& fname, WritableFile** result) {
    Status s = target()->NewWritableFile(fname, result);
    if (!s.ok()) {
        return s;
    }

    std::string baseName = fname.substr(fname.rfind('/') + 1);
    uint64_t number;
    FileType type;
    ParseFileName(baseName, &number, &type);
    if (type != kLogFile || !logRotate() || 0 == maxLogFiles_) {
        return s;
    }

    FDSGUARD(rotateMtx_);

    std::string prefix = logDirName() + logFilePrefix();
    for (fds_int32_t i = maxLogFiles() - 1; i >= 0; --i) {
        std::string name = prefix + "." + std::to_string(i);
        if (!target()->FileExists(name)) {
            continue;
        }

        if (static_cast<fds_int32_t>(maxLogFiles_ - 1) == i) {
            s = target()->DeleteFile(name);
            fds_verify(s.ok());
            continue;
        }

        std::string newName = prefix + "." + std::to_string(i + 1);
        s = target()->RenameFile(name, newName);
        fds_verify(s.ok());
    }

    if (target()->FileExists(prefix)) {
        std::string newName = prefix + "." + std::to_string(0);
        s = target()->RenameFile(prefix, newName);
        fds_verify(s.ok());

        if (timelineEnable_) {
           std::string archiveFile = logDirName() + archivePrefix() + "." +
                std::to_string(util::getTimeStampMicros());
           fds_verify(0 == link(newName.c_str(), archiveFile.c_str()));
        }
    }

    fds_verify(0 == link(fname.c_str(), prefix.c_str()));

    return s;
}

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


Status CopyEnv::DeleteDir(const std::string & dir) {
    // Get consistent view of all files.
    mtx_.lock();

    Status s;
    if (copying_ || !deferredDeletions_.empty()) {
        mtx_.unlock();
        GLOGERROR << "Copying going on for the directory " << dir;
        return Status::IOError("delete directory failed");
    }

    std::vector<std::string> files;
    s = GetChildren(dir, &files);
    if (!s.ok()) {
        mtx_.unlock();
        GLOGERROR << "GetChildren failed with status " << s.ToString();
        return s;
    }

    std::vector<fds_uint64_t> lengths(files.size());
    for (size_t i = 0; i < files.size(); i++) {
        if ("." == files[i] || ".." == files[i]) {
            continue;
        }

        s = target()->DeleteFile(dir + "/" + files[i]);
        if (!s.ok()) {
            mtx_.unlock();
            GLOGERROR << "Could not delete file " << files[i] << " status " << s.ToString();
        }
    }

    target()->DeleteDir(dir);
    if (!s.ok()) {
        mtx_.unlock();
        GLOGERROR << "Could not delete directory " << dir << " status " << s.ToString();
        return s;
    }

    mtx_.unlock();
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
