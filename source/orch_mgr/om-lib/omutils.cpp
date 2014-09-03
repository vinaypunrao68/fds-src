/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <omutils.h>
#include <fds_uuid.h>
#include <util/stringutils.h>

namespace fds {
// TODO(prem): mkae it use lower case soon
fds_uint64_t getUuidFromVolumeName(const std::string& name) {
    // std::string lowerName = fds::util::strlower(name);
    return fds_get_uuid64(name);
}

fds_uint64_t getUuidFromResourceName(const std::string& name) {
    // std::string lowerName = fds::util::strlower(name);
    return fds_get_uuid64(name);
}

}  // namespace fds
