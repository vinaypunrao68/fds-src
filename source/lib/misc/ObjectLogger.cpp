/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <string>

#include <boost/scoped_ptr.hpp>

#include <fds_module.h>
#include <fds_process.h>
#include <ObjectLogger.h>
#include <serialize.h>

namespace fds {

const fds_uint32_t ObjectLogger::FILE_SIZE;
const fds_uint32_t ObjectLogger::MAX_FILES;

void ObjectLogger::reset() {
    if (init_) {
        closeFile();
        unlink(filename_.c_str());
    }

    for (fds_int32_t i = maxFiles_ - 1; i >= 0; --i) {
        std::string name = filename(i);
        if (name.empty()) {
            continue;   // file not found
        }

        unlink(name.c_str());
    }

    init_ = false;
}

void ObjectLogger::rotate() {
    if (init_) {
        for (fds_int32_t i = maxFiles_ - 1; i >= 0; --i) {
            std::string name = filename(i);
            if (name.empty()) {
                continue;   // file not found
            }

            if (static_cast<fds_int32_t>(maxFiles_ - 1) == i) {
                fds_verify(0 == unlink(name.c_str()));
                continue;
            }

            std::string newName = filename_ + "." + std::to_string(i + 1);
            fds_verify(0 == rename(name.c_str(), newName.c_str()));
        }

        closeFile();
        if (0 == maxFiles_) {
            fds_verify(0 == unlink(filename_.c_str()));
        } else {
            std::string newName = filename_ + "." + std::to_string(0);
            fds_verify(0 == rename(filename_.c_str(), newName.c_str()));
        }
        openFile();
    }
}

off_t ObjectLogger::log(const serialize::Serializable * obj) {
    if (!init_) {
        init();
    }

    off_t pos = currentPos();
    if (pos >= filesize_) {
        return -1;  // current file is full, rotation needed
    }

    obj->write(s_.get());
    return pos;
}
}   // namespace fds
