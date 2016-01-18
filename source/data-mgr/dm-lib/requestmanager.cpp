/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <DataMgr.h>
#include <requestmanager.h>
namespace fds { namespace dm {

RequestManager::RequestManager(DataMgr* dataMgr): dataMgr(dataMgr) {
}

Error RequestManager::sendReloadVolumeRequest(const NodeUuid & nodeId, const fds_volid_t & volId) {
    auto asyncReq = gSvcRequestPool->newEPSvcRequest(nodeId.toSvcUuid());

    boost::shared_ptr<fpi::ReloadVolumeMsg> msg = boost::make_shared<fpi::ReloadVolumeMsg>();
    msg->volume_id = volId.get();
    asyncReq->setPayload(FDSP_MSG_TYPEID(fpi::ReloadVolumeMsg), msg);

    SvcRequestCbTask<EPSvcRequest, fpi::ReloadVolumeRspMsg> waiter;
    asyncReq->onResponseCb(waiter.cb);

    asyncReq->invoke();
    waiter.await();
    return waiter.error;
}

Error RequestManager::sendLoadFromArchiveRequest(const NodeUuid & nodeId, const fds_volid_t & volId, const std::string& fileName) {
    auto asyncReq = gSvcRequestPool->newEPSvcRequest(nodeId.toSvcUuid());

    SHPTR<fpi::LoadFromArchiveMsg> msg(new fpi::LoadFromArchiveMsg());
    msg->volId = volId.get();
    msg->filename = fileName;
    asyncReq->setPayload(FDSP_MSG_TYPEID(fpi::LoadFromArchiveMsg), msg);
    
    SvcRequestCbTask<EPSvcRequest, fpi::LoadFromArchiveMsg> waiter;
    asyncReq->onResponseCb(waiter.cb);

    asyncReq->invoke();
    waiter.await();
    return waiter.error;
}

void RequestManager::sendEventMessageToOM(fpi::EventType eventType,
                                          fpi::EventCategory eventCategory,
                                          fpi::EventSeverity eventSeverity,
                                          fpi::EventState eventState,
                                          const std::string& messageKey,
                                          std::vector<fpi::MessageArgs> messageArgs,
                                          const std::string& messageFormat) {
    fpi::NotifyEventMsgPtr eventMsg(new fpi::NotifyEventMsg());

    eventMsg->event.type = eventType;
    eventMsg->event.category = eventCategory;
    eventMsg->event.severity = eventSeverity;
    eventMsg->event.state = eventState;

    auto now = std::chrono::system_clock::now();
    fds_uint64_t seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    eventMsg->event.initialTimestamp = seconds;
    eventMsg->event.modifiedTimestamp = seconds;
    eventMsg->event.messageKey = messageKey;
    eventMsg->event.messageArgs = messageArgs;
    eventMsg->event.messageFormat = messageFormat;
    eventMsg->event.defaultMessage = "DEFAULT MESSAGE";

    auto svcMgr = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
    auto request = svcMgr->newEPSvcRequest (MODULEPROVIDER()->getSvcMgr()->getOmSvcUuid());

    request->setPayload(FDSP_MSG_TYPEID(fpi::NotifyEventMsg), eventMsg);
    request->invoke();

}

void RequestManager::sendHealthCheckMsgToOM(fpi::HealthState serviceState,
                                            fds_errno_t statusCode,
                                            const std::string& statusInfo) {
    fpi::SvcInfo info = MODULEPROVIDER()->getSvcMgr()->getSelfSvcInfo();

    // Send health check thrift message to OM
    fpi::NotifyHealthReportPtr healthRepMsg(new fpi::NotifyHealthReport());
    healthRepMsg->healthReport.serviceInfo.svc_id = info.svc_id;
    healthRepMsg->healthReport.serviceInfo.name = info.name;
    healthRepMsg->healthReport.serviceInfo.svc_port = info.svc_port;
    healthRepMsg->healthReport.serviceState = serviceState;
    healthRepMsg->healthReport.statusCode = statusCode;
    healthRepMsg->healthReport.statusInfo = statusInfo;

    auto svcMgr = MODULEPROVIDER()->getSvcMgr()->getSvcRequestMgr();
    auto request = svcMgr->newEPSvcRequest (MODULEPROVIDER()->getSvcMgr()->getOmSvcUuid());

    request->setPayload (FDSP_MSG_TYPEID (fpi::NotifyHealthReport), healthRepMsg);
    request->invoke();

}


}  // namespace dm
}  // namespace fds
