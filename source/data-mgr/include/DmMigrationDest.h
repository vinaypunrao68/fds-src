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
#include <DmMigrationExecutor.h>

namespace fds {

class DmMigrationDest : public DmMigrationExecutor {
public:
    DmMigrationDest(int64_t _migrId,
                    DataMgr &_dm,
                    NodeUuid &_srcDmUuid,
                    fpi::FDSP_VolumeDescType& _volDesc,
                    uint32_t _timeout) :
        DmMigrationExecutor(_dm,
                            _srcDmUuid,
                            _volDesc,
                            _migrId,
                            false,
                            NULL,
                            _timeout) {}
    ~DmMigrationDest() {}
    typedef std::shared_ptr<DmMigrationDest> shared_ptr;
    typedef std::unique_ptr<DmMigrationDest> unique_ptr;
private:
};


} // namespace fds

#endif // SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONDEST_H_