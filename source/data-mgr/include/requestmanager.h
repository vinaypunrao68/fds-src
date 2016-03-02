/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_REQUESTMANAGER_H_
#define SOURCE_DATA_MGR_INCLUDE_REQUESTMANAGER_H_
#include <fds_typedefs.h>
#include <fds_error.h>
#include <fdsp/event_types_types.h>
namespace fds {
struct DataMgr;
namespace dm {
struct RequestManager {
    explicit RequestManager(DataMgr* dataMgr);
    Error sendReloadVolumeRequest(const NodeUuid & nodeId, const fds_volid_t & volId);
    Error sendLoadFromArchiveRequest(const NodeUuid & nodeId, const fds_volid_t & volId, const std::string& fileName);
    Error sendArchiveVolumeRequest(const NodeUuid & nodeId, const fds_volid_t & volid);
    /**
     * Send event message to OM
     */
    void sendEventMessageToOM(fpi::EventType eventType,
                              fpi::EventCategory eventCategory,
                              fpi::EventSeverity eventSeverity,
                              fpi::EventState eventState,
                              const std::string& messageKey,
                              std::vector<fpi::MessageArgs> messageArgs,
                              const std::string& messageFormat);

    // Send health check message
    void sendHealthCheckMsgToOM(fpi::HealthState serviceState,
                                fds_errno_t statusCode,
                                const std::string& statusInfo);


  protected:
    DataMgr* dataMgr;
};
}  // namespace dm
}  // namespace fds

#endif  // SOURCE_DATA_MGR_INCLUDE_REQUESTMANAGER_H_
