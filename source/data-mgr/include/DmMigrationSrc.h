/*
 * Copyright 2015 Formation Data Systems, Inc.
 */

#ifndef SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONSRC_H_
#define SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONSRC_H_

#include <util/Log.h>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <MigrationUtility.h>
#include <dmhandler.h>
#include <DmMigrationBase.h>

namespace fds {


class DmMigrationSrc : DmMigrationBase {
public:
    using DmMigrationBase::DmMigrationBase;
    ~DmMigrationSrc() {}
    typedef std::shared_ptr<DmMigrationSrc> shared_ptr;
    typedef std::unique_ptr<DmMigrationSrc> unique_ptr;
private:
};


} // namespace fds
#endif // SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONSRC_H_
