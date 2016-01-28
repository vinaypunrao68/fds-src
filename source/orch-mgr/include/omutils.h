/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OMUTILS_H_
#define SOURCE_ORCH_MGR_INCLUDE_OMUTILS_H_
#include <string>
#include <fds_types.h>
#include <kvstore/kvstore.h>
#include <kvstore/configdb.h>


namespace fds 
{
    namespace fpi = FDS_ProtocolInterface;

    /*
     * Used throughout multiple places
     * TODO - whoever uses this macro needs to be fixed. We should be updating configDB
     * based on the whole SvcInfo, (UUID + incarnation No), instead of just uuid.
     * Once that is done, we can change this macro to take in SvcInfo as an argument
     * instead of just a UUID.
     */
    #define UPDATE_CONFIGDB_SERVICE_STATE(configdbPtr, uuid, status) \
            fpi::SvcInfoPtr svcInfoPtr = boost::make_shared<fpi::SvcInfo>(); \
            fds_mutex::scoped_lock l(dbLock); \
            svcInfoPtr = boost::make_shared<fpi::SvcInfo>(); \
            svcInfoPtr->svc_id.svc_uuid.svc_uuid = uuid.svc_uuid; \
            svcInfoPtr->svc_status = status; \
            fds::change_service_state(configdbPtr, \
                                      svcInfoPtr, \
                                      status, \
                                      false);

    /**
     * This is case insensitive
     */
    fds_uint64_t getUuidFromVolumeName(const std::string& name);
    fds_uint64_t getUuidFromResourceName(const std::string& name);

    void change_service_state( kvstore::ConfigDB* configDB,
                               const fds_uint64_t svc_uuid, 
                               const fpi::ServiceStatus svc_status );

    /**
     * Changes the service state of a specific service.
     * If the incarnation number is given as part of svcInfo, it will update
     * and check to see if the specified operation has completed successfully.
     * If it has completed, then will it persist the changes into configDB.
     */
    void change_service_state( kvstore::ConfigDB* configDB,
                               const fpi::SvcInfoPtr svcInfo,
                               const fpi::ServiceStatus svc_status,
                               const bool updateSvcMap );
    bool isSameSvcInfoInstance( fpi::SvcInfo svcInfo );
    fpi::FDSP_Node_Info_Type fromSvcInfo( const fpi::SvcInfo& svcinfo );
    fpi::FDSP_NodeState fromServiceStatus(fpi::ServiceStatus svcStatus);
    
    void updateSvcInfoList(std::vector<fpi::SvcInfo>& svcInfos,
                           bool smFlag, bool dmFlag, bool amFlag);
    std::vector<fpi::SvcInfo>::iterator isServicePresent(
                                          std::vector<fpi::SvcInfo>& svcInfos,
                                          FDS_ProtocolInterface::FDSP_MgrIdType type);
    fpi::SvcInfo getNewSvcInfo(FDS_ProtocolInterface::FDSP_MgrIdType type);
    void getServicesToStart(bool start_sm, bool start_dm,
                            bool start_am,kvstore::ConfigDB* configDB,
                            NodeUuid nodeUuid,
                            std::vector<fpi::SvcInfo>& svcInfoList);
    void retrieveSvcId(int64_t pmID, fpi::SvcUuid& svcUuid,
                       FDS_ProtocolInterface::FDSP_MgrIdType type);

}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_OMUTILS_H_
