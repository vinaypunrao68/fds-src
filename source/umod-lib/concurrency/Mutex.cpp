/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <concurrency/Mutex.h>

#include <string>

namespace fds {

fds_mutex::fds_mutex()
        : _name("Default mutex name") {
    _m = new boost::mutex();
}

fds_mutex::fds_mutex(const std::string& name)
    : _name(name) {
    _m = new boost::mutex();
}

fds_mutex::fds_mutex(const char *name)
    : _name(name) {
    _m = new boost::mutex();
}

fds_mutex::~fds_mutex() {
    delete _m;
}

std::string fds_mutex::get_name() const {
    return _name;
}

void fds_mutex::lock() {
    _m->lock();
}

void fds_mutex::unlock() {
    _m->unlock();
}

bool fds_mutex::try_lock() {
    return _m->try_lock();
}

}  // namespace fds
