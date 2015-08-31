/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <fds_error.h>
#include <string>
#include <fds_defines.h>
namespace fds {

std::map<int, enumDescription>& operator, (std::map<int, enumDescription>& dest,
                                           const std::pair<int, enumDescription>& entry) {
    dest[entry.first] = entry.second;
    return dest;
}

std::map<int, enumDescription> fds_errno_desc;
MAKE_ENUM_MAP_GLOBAL(fds_errno_desc, FDS_ERRORNO_ENUM_VALUES);

std::ostream& operator<<(std::ostream& out, const Error& err) {
    return out << "Error code: " << std::to_string(err.GetErrno()) << ": " << err.GetErrName() << ": " << err.GetErrstr();
}

std::ostream& operator<<(std::ostream& os, FDSN_Status status) {
    auto enumDesc = fds_errno_desc.find(status);
    if (enumDesc == fds_errno_desc.end()) {
        os << "unknown status code (" << static_cast<int>(status) <<")";
    } else {
        os << enumDesc->second.first << ": " << enumDesc->second.second;
    }

    return os;
}

std::string toString(FDSN_Status status) {
    auto enumDesc = fds_errno_desc.find(status);
    if (enumDesc == fds_errno_desc.end()) {
        return Error(status).GetErrstr();
    } else {
        return enumDesc->second.second;
    }
}

Exception::Exception(const Error& e, const std::string& message) throw()
        : err(e), message(message) {
    if (message.empty()) {
        this->message = e.GetErrstr();
    }
}

Exception::Exception(const std::string& message) throw()
        : err(ERR_INVALID) , message(message) {
}

Exception::Exception(const Exception& e) throw() {
    err = e.err;
    message = e.message;
}

Exception& Exception::operator= (const Exception& e) throw() {
    err = e.err;
    message = e.message;
    return *this;
}

const Error& Exception::getError() const {
    return err;
}

Exception::~Exception() throw() {}

const char* Exception::what() const throw() {
    return message.c_str();
}

}  // namespace fds
