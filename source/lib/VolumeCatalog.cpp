/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <string>

#include "VolumeCatalog.h"

namespace fds {

VolumeCatalog::VolumeCatalog(const std::string& _file)
    : Catalog(_file) {
}

VolumeCatalog::~VolumeCatalog() {
}

TimeCatalog::TimeCatalog(const std::string& _file)
    : Catalog(_file) {
}

TimeCatalog::~TimeCatalog() {
}

}  // namespace fds
