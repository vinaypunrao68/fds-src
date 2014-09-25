/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <string>

#include "VolumeCatalog.h"

namespace fds {

VolumeCatalog::VolumeCatalog(const std::string& _file, fds_bool_t cat_flag)
    : Catalog(_file) {
}

VolumeCatalog::~VolumeCatalog() {
}

TimeCatalog::TimeCatalog(const std::string& _file, fds_bool_t cat_flag)
    : Catalog(_file) {
}

TimeCatalog::~TimeCatalog() {
}

}  // namespace fds
