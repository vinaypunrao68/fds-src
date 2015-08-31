/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

// Standard includes.
#include <iostream>
#include <string>

// Internal includes.
#include "leveldb/db.h"
#include "fds_types.h"

int main(int argc, char *argv[]) {
  std::string key("My key");
  std::string data("My data");

  for (fds_uint32_t i = 0; i < 10; i++)
  {
    leveldb::Slice rec { key };

    std::cout << "Record size is " << rec.size() << std::endl;
  }

  return 0;
}
