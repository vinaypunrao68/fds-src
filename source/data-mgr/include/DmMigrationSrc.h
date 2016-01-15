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
#include <DmMigrationClient.h>

namespace fds {

class DmMigrationSrc : public DmMigrationClient {
public:
    typedef std::shared_ptr<DmMigrationSrc> shared_ptr;
    typedef std::unique_ptr<DmMigrationSrc> unique_ptr;

    DmMigrationSrc(DataMgr& _dataMgr,
                   const NodeUuid& _myUuid,
                   NodeUuid& _destDmUuid,
                   int64_t migrationId,
                   fpi::CtrlNotifyInitialBlobFilterSetMsgPtr _ribfsm,
                   DmMigrationClientDoneHandler _handle,
                   migrationCb _cleanup,
                   uint64_t _maxDeltaBlobs,
                   uint64_t _maxDeltaBlobDesc,
                   int32_t _volmetaVersion);
    ~DmMigrationSrc();
    void abortMigration() override;
    void run() override;
    /* This message is sent at the end of migration */
    void sendFinishStaticMigrationMsg(const Error &e);

    /*xxx: Get rid of the following forwarding overrides as part migration
     * clean up
     */
    void turnOnForwarding() override;
    void turnOffForwarding() override;
    fds_bool_t shouldForwardIO(fds_uint64_t dmtVersionIn) override;
};


} // namespace fds
#endif // SOURCE_DATA_MGR_INCLUDE_DMMIGRATIONSRC_H_
