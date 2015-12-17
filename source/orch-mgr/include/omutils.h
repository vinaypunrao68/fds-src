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
    
    /**
     * This is case insensitive
     */
    fds_uint64_t getUuidFromVolumeName(const std::string& name);
    fds_uint64_t getUuidFromResourceName(const std::string& name);

    void change_service_state( kvstore::ConfigDB* configDB,
                               const fds_uint64_t svc_uuid, 
                               const fpi::ServiceStatus svc_status );
    void change_service_state( kvstore::ConfigDB* configDB,
                               const fds_uint64_t svc_uuid, 
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
