/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONDEST_H_
#define SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONDEST_H_

#include <util/Log.h>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <MigrationUtility.h>
#include <dmhandler.h>
#include <DmMigrationBase.h>

namespace fds {

class DmMigrationDest : DmMigrationBase {
public:
    ~DmMigrationDest();
    typedef std::shared_ptr<DmMigrationDest> shared_ptr;
private:
};


} // namespace fds

#endif // SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONDEST_H_
