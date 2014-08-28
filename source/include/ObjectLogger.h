/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_OBJECTLOGGER_H_
#define SOURCE_INCLUDE_OBJECTLOGGER_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string>

#include <fds_types.h>
#include <serialize.h>
#include <concurrency/Mutex.h>

namespace fds {

/**
 * Logs serializable objects in a file with log-rotate
 */
class ObjectLogger {
  public:
    typedef boost::shared_ptr<ObjectLogger> ptr;
    typedef boost::shared_ptr<const ObjectLogger> const_ptr;

    static const fds_uint32_t FILE_SIZE = 20971520;
    static const fds_uint32_t MAX_FILES = 10;

    // ctor & dtor
    ObjectLogger(const std::string & filename, fds_uint32_t filesize = FILE_SIZE,
            fds_uint32_t maxFiles = MAX_FILES);

    virtual ~ObjectLogger() {
        closeFile();
    }

    inline const std::string & filename() const {
        return filename_;
    }

    inline const fds_uint32_t filesize() const {
        return filesize_;
    }

    inline const fds_uint32_t maxFiles() const {
        return maxFiles_;
    }

    std::string getFile(fds_uint32_t index) const;

    // log operation
    virtual void log(const serialize::Serializable * obj);

    // retrieve operations
    // TODO(umesh): implement this

  private:
    std::string filename_;
    fds_uint32_t filesize_;
    fds_uint32_t maxFiles_;

    fds_int32_t fd_;
    boost::shared_ptr<serialize::Serializer> s_;

    fds_mutex logLock_;

    // Methods
    inline bool isFull() {
        off_t pos = lseek(fd_, 0, SEEK_END);
        fds_assert(-1 != pos);
        return (pos >= filesize_);
    }

    inline void closeFile() {
        close(fd_);
        s_.reset();
    }

    inline void openFile() {
        if (-1 == (fd_ = open(filename_.c_str(), O_RDWR | O_SYNC | O_CREAT | O_APPEND,
                S_IRWXU | S_IRGRP))) {
            GLOGWARN << "Failed to open file " << filename_;
            throw std::system_error(errno, std::system_category());
        }
        s_.reset(serialize::getFileSerializer(filename_));
    }

    void rotate();
};
}  /* namespace fds */
#endif  // SOURCE_INCLUDE_OBJECTLOGGER_H_
