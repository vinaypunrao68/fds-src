/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Object database class. The object database is a key-value store
 * that provides local objec storage.
 */
#ifndef SOURCE_STOR_MGR_ODB_H_
#define SOURCE_STOR_MGR_ODB_H_

#include <iostream>  // NOLINT(*)

#include "stor_mgr_err.h"

namespace osm {

  class ObjectDB {
 public:
    /*
     * Default constructor
     */
    ObjectDB();

    /*
     * Default destructor
     */
    ~ObjectDB();

    fds_sm_err_t open();

 private:
  };

}  // namespace osm

#endif  // SOURCE_STOR_MGR_ODB_H_
