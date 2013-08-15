/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <iostream> // NOLINT(*)
#include <string>

#include "lib/Catalog.h"

int main(int argc, char *argv[]) {
  std::string key("My key");
  std::string data("My data");
  
  fds::Record *rec;

  for (fds_uint32_t i = 0; i < 10; i++) {
    rec = new fds::Record(key);

    std::cout << "Record size is " << rec->size() << std::endl;

    delete rec;
  }

  return 0;
}
