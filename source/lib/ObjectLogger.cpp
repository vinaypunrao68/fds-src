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

ObjectLogger::ObjectLogger(const std::string & filename, fds_uint32_t filesize /* = FILE_SIZE */,
        fds_uint32_t maxFiles /* = MAX_FILES */) : filename_(filename), filesize_(filesize),
        maxFiles_(maxFiles) {
    openFile();
}

void ObjectLogger::rotate() {
    for (fds_int32_t i = maxFiles_ - 1; i >= 0; --i) {
        std::string name = getFile(i);
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
    std::string newName = filename_ + "." + std::to_string(0);
    fds_verify(0 == rename(filename_.c_str(), newName.c_str()));
    openFile();
}

std::string ObjectLogger::getFile(fds_uint32_t index) const {
    std::string name = filename_ + "." + std::to_string(index);
    struct stat buffer = {0};
    return (index < maxFiles_ && !stat(name.c_str(), &buffer) ? name : std::string());
}

void ObjectLogger::log(const serialize::Serializable * obj) {
    FDSGUARD(logLock_);

    if (isFull()) {
        rotate();
    }
    obj->write(s_.get());
}
}   // namespace fds
