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
                    DataMgr *_dm,
                    NodeUuid &_srcDmUuid,
                    fpi::FDSP_VolumeDescType& _volDesc,
                    uint32_t _timeout,
                    migrationCb cbToCoordinator) :
        DmMigrationExecutor(_dm,
                            _srcDmUuid,
                            _volDesc,
                            _migrId,
                            false,
                            std::bind(&DmMigrationDest::staticMigrationStatusToSrc,
                                      this,
                                      std::placeholders::_1,
                                      std::placeholders::_2,
                                      std::placeholders::_3,
                                      cbToCoordinator),
                            _timeout,
                            true) {}
    ~DmMigrationDest() {}

    void staticMigrationStatusToSrc(NodeUuid srcNodeUuid,
                                    fds_volid_t volumeId,
                                    const Error &result,
                                    migrationCb cbToCoordinator);

    /**
     * Wraps around processInitialBlobFilterSet, now done in a volume
     * specific context.
     */
    Error start();

    typedef std::shared_ptr<DmMigrationDest> shared_ptr;
    typedef std::unique_ptr<DmMigrationDest> unique_ptr;
private:
};


} // namespace fds

#endif // SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONDEST_H_
