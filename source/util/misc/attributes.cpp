/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <util/Log.h>
#include <map>
namespace fds {
namespace util {

Attributes::Attributes() : Properties() {
}

Attributes::~Attributes() {
}

void Attributes::setVolId(const std::string& key, const fds_volid_t volId) {
    setInt(key, volId.get());
}

fds_volid_t Attributes::getVolId(const std::string& key) {
    return fds_volid_t(getInt(key));
}


boss& operator<<(boss& out, const Attributes& attr) {
    for (auto const& item : *attr.data) {
        out << item.first << ":" << item.second << " ";
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, const Attributes& attr) {
    for (auto const& item : *attr.data) {
        out << item.first << ":" << item.second << " ";
    }
    return out;
}

}  // namespace util
}  // namespace fds
