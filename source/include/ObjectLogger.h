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

    template<typename T>
    class Iterator {
      public:
        ~Iterator() {
            d_.reset();
            if (-1 != fd_) {
                close(fd_);
            }
            valid_ = false;
        }

        void startAt(off_t offset = 0) {
            if (!valid_) {
                return;
            } else if (lseek(fd_, offset, SEEK_SET) != offset) {
                valid_ = false;
            } else {
                offset_ = offset;
                d_.reset(serialize::getFileDeserializer(fd_));
                nextOffset_ = offset_ + read();
            }
        }

        fds_bool_t valid() const {
            return valid_;
        }

        boost::shared_ptr<T> value() {
            return value_;
        }

        off_t offset() const {
            return offset_;
        }

        void next() {
            if (!valid_) {
                return;
            }

            offset_ = nextOffset_;
            nextOffset_ = offset_ + read();
        }

      private:
        ObjectLogger * logger_;
        fds_int32_t index_;
        std::string filename_;
        fds_int32_t fd_;
        boost::shared_ptr<serialize::Deserializer> d_;
        fds_bool_t valid_;
        off_t offset_;
        off_t nextOffset_;

        boost::shared_ptr<T> value_;

        friend ObjectLogger;

        // ctor
        Iterator(ObjectLogger * logger, fds_int32_t index) : logger_(logger), index_(index),
                filename_(logger->filename(index)), fd_(-1), d_(0), valid_(false), offset_(0),
                nextOffset_(0) {
            if (!filename_.empty()) {
                if (-1 == (fd_ = open(filename_.c_str(), O_RDONLY))) {
                    GLOGWARN << "Failed to open file " << filename_;
                    throw std::system_error(errno, std::system_category());
                }

                valid_ = true;
                startAt(0);
            }
        }

        fds_uint32_t read() {
            fds_uint32_t bytes = 0;

            if (valid_) {
                value_.reset(new T());
                try {
                    bytes = value_->read(d_.get());
                } catch(std::exception & ex) {
                    // reached end of file
                    value_.reset();
                    valid_ = false;
                }
            }

            return bytes;
        }
    };

    // ctor & dtor
    ObjectLogger(const std::string & filename, fds_uint32_t filesize = FILE_SIZE,
            fds_uint32_t maxFiles = MAX_FILES) : filename_(filename), filesize_(filesize),
            maxFiles_(maxFiles) {}

    virtual ~ObjectLogger() {
        if (init_) {
            closeFile();
        }
    }

    /**
     * Get filename
     * @return string empty string , if the file with given index doesn't exist
     */
    inline const std::string filename(fds_int32_t index = -1) const {
        std::string name = index < 0 ? filename_ : filename_ + "." + std::to_string(index);
        struct stat buffer = {0};
        return (!stat(name.c_str(), &buffer) ? name : std::string());
    }

    inline const fds_uint32_t filesize() const {
        return filesize_;
    }

    inline const fds_uint32_t maxFiles() const {
        return maxFiles_;
    }

    /**
     * Reset log
     * Removes all existing log files and starts fresh
     */
    void reset();

    void rotate();

    // log operation
    virtual off_t log(const serialize::Serializable * obj);

    virtual off_t LOG(const serialize::Serializable *obj) {
        off_t pos = log(obj);
        if (pos < 0) {
            rotate();
        }
        return log(obj);
    }

    // retrieve operations
    template<typename T>
    boost::shared_ptr<ObjectLogger::Iterator<T> > newIterator(fds_int32_t index) {
        return boost::shared_ptr<ObjectLogger::Iterator<T> >(
                new ObjectLogger::Iterator<T>(this, index));
    }

  protected:
    std::string filename_;
    fds_uint32_t filesize_;
    fds_uint32_t maxFiles_;

    // Methods
    inline void init() {
        if (!init_) {
            openFile();
            init_ = true;
        }
    }

  private:
    fds_bool_t init_;
    fds_int32_t fd_;
    boost::shared_ptr<serialize::Serializer> s_;

    // Methods
    inline off_t currentPos() {
        off_t pos = lseek(fd_, 0, SEEK_END);
        fds_assert(-1 != pos);
        return pos;
    }

    inline void openFile() {
        if (-1 == (fd_ = open(filename_.c_str(), O_RDWR | O_SYNC | O_CREAT | O_APPEND,
                S_IRWXU | S_IRGRP))) {
            GLOGWARN << "Failed to open file " << filename_;
            throw std::system_error(errno, std::system_category());
        }
        s_.reset(serialize::getFileSerializer(fd_));
    }

    inline void closeFile() {
        close(fd_);
        s_.reset();
    }
};
}  /* namespace fds */
#endif  // SOURCE_INCLUDE_OBJECTLOGGER_H_
