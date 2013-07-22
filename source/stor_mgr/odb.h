/**
 * Object database class. The object database is a key-value store
 * that provides local objec storage.
 */
#ifndef FDS_OBJECT_DATABASE_H_
#define FDS_OBJECT_DATABASE_H_

#include <iostream>

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

} // namespace osm

#endif // FDS_OBJECT_DATABASE_H_
