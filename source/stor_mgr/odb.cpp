/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include "stor_mgr/odb.h"

namespace osm {

/**
 * Constructor
 */
ObjectDB::ObjectDB() {
  std::cout << "Running the constructor" << std::endl;
}

ObjectDB::~ObjectDB() {
  std::cout << "Running the destructor" << std::endl;
}

fds_sm_err_t
ObjectDB::open() {
  std::cout << "Running open()" << std::endl;
}

}  // namespace osm
