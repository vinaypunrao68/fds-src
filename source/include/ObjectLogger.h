/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_OBJECTLOGGER_H_
#define SOURCE_INCLUDE_OBJECTLOGGER_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string>
#include <vector>
#include <limits>

#include <boost/scoped_ptr.hpp>

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
            fds_uint32_t maxFiles = MAX_FILES, fds_bool_t clear = false);

    virtual ~ObjectLogger();

    inline const std::string & filename() const {
        return filename_;
    }

    inline const fds_uint32_t filesize() const {
        return filesize_;
    }

    inline const fds_uint32_t maxFiles() const {
        return maxFiles_;
    }

    // log operation
    virtual void log(const serialize::Serializable * obj);

    // retrieve operations
    template<typename T>
    fds_uint32_t getObjects(fds_int32_t fileIndex,
            std::vector<boost::shared_ptr<T> > & objects,
            fds_uint32_t max = std::numeric_limits<fds_uint32_t>::max()) {
        fds_uint32_t count = 0;

        fds_mutex & lockRef_ = (-1 == fileIndex ? logLock_ : rotateLock_);
        FDSGUARD(lockRef_);

        const std::string & name = (-1 == fileIndex ? filename_ : getFile(fileIndex));

        if (!name.empty()) {
            boost::scoped_ptr<serialize::Deserializer> d(serialize::getFileDeserializer(name));

            while (count < max) {
                boost::shared_ptr<T> tmp(new T());
                try {
                    tmp->read(d.get());
                    objects.push_back(tmp);
                    ++count;
                } catch(std::exception & e) {
                    // reached end of file
                    break;
                }
            }
        }

        return count;
    }

  private:
    std::string filename_;
    fds_uint32_t filesize_;
    fds_uint32_t maxFiles_;
    fds_bool_t clear_;

    fds_int32_t fd_;
    boost::shared_ptr<serialize::Serializer> s_;
    boost::shared_ptr<serialize::Deserializer> d_;

    fds_mutex logLock_;
    fds_mutex rotateLock_;

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
        d_.reset(serialize::getFileDeserializer(filename_));
    }

    std::string getFile(fds_uint32_t index) const {
        std::string name = filename_ + "." + std::to_string(index);
        struct stat buffer = {0};
        return (index < maxFiles_ && !stat(name.c_str(), &buffer) ? name : std::string());
    }

    void rotate();
};
}  /* namespace fds */
#endif  // SOURCE_INCLUDE_OBJECTLOGGER_H_
