/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <AMSvcHandler.h>

#include <fdsp/dm_api_types.h>
#include "net/SvcRequest.h"
#include "AmProcessor.h"
#include "AmVolume.h"
#include "requests/DetachVolumeReq.h"
#include "fds_process.h"

namespace fds {

AMSvcHandler::~AMSvcHandler() {}
AMSvcHandler::AMSvcHandler(CommonModuleProviderIf *provider,
                           std::shared_ptr<AmProcessor> processor)
// NOTE: SMSvcHandler should take fds_module_provider as a param so that we don't need
// any globals
    : PlatNetSvcHandler(provider),
      amProcessor(processor)
{
    REGISTER_FDSP_MSG_HANDLER(fpi::NodeSvcInfo, notifySvcChange);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyThrottle, SetThrottleLevel);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyQoSControl, QoSControl);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyVolMod, NotifyModVol);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyVolAdd, AddVol);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyVolRemove, RemoveVol);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyDLTUpdate, NotifyDLTUpdate);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyDMTUpdate, NotifyDMTUpdate);
    REGISTER_FDSP_MSG_HANDLER(fpi::PrepareForShutdownMsg, shutdownAM);
    REGISTER_FDSP_MSG_HANDLER(fpi::AddToVolumeGroupCtrlMsg, addToVolumeGroup);
    REGISTER_FDSP_MSG_HANDLER(fpi::SwitchCoordinatorMsg, switchCoordinator);
}

// notifySvcChange
// ---------------
//
void
AMSvcHandler::notifySvcChange(boost::shared_ptr<fpi::AsyncHdr>    &hdr,
                              boost::shared_ptr<fpi::NodeSvcInfo> &msg)
{
#if 0
    DomainAgent::pointer self;

    self = NetPlatform::nplat_singleton()->nplat_self();
    fds_verify(self != NULL);

    auto list = msg->node_svc_list;
    for (auto it = list.cbegin(); it != list.cend(); ++it) {
        auto rec = *it;
        if (rec.svc_type == fpi::FDSP_DATA_MGR) {
            self->agent_fsm_input(msg, rec.svc_type,
                                  rec.svc_runtime_state, rec.svc_deployment_state);
        }
    }
#endif
}

void
AMSvcHandler::QoSControl(boost::shared_ptr<fpi::AsyncHdr>           &hdr,
                     boost::shared_ptr<fpi::CtrlNotifyQoSControl> &msg)
{
    Error err(ERR_OK);

    LOGNORMAL << "qos ctrl total rate:" << msg->qosctrl.total_rate;

    if (amProcessor->isShuttingDown())
    {
        LOGDEBUG << "request rejected due to shutdown";
        err = ERR_SHUTTING_DOWN;
    } else {
        auto rate = msg->qosctrl.total_rate;
        err = amProcessor->updateQoS(&rate, nullptr);
    }

    hdr->msg_code = err.GetErrno();
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyQoSControl), *msg);
}

// SetThrottleLevel
// ----------------
//
void
AMSvcHandler::SetThrottleLevel(boost::shared_ptr<fpi::AsyncHdr>           &hdr,
                               boost::shared_ptr<fpi::CtrlNotifyThrottle> &msg)
{
    Error err(ERR_OK);

    LOGNORMAL << "set throttle:" << msg->throttle.throttle_level;

    if (amProcessor->isShuttingDown()) {
        LOGDEBUG << "request rejected due to shutdown";
        err = ERR_SHUTTING_DOWN;
    } else {
        float throttle_level = msg->throttle.throttle_level;
        err = amProcessor->updateQoS(nullptr, &throttle_level);
    }

    hdr->msg_code = err.GetErrno();
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyThrottle), *msg);
}

// NotifyModVol
// ------------
//
void
AMSvcHandler::NotifyModVol(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                           boost::shared_ptr<fpi::CtrlNotifyVolMod> &vol_msg)
{
    Error err(ERR_OK);

    fds_volid_t vol_uuid (vol_msg->vol_desc.volUUID);
    VolumeDesc vdesc(vol_msg->vol_desc);
    GLOGDEBUG << vdesc << "received modify event from OM";

    if (amProcessor->isShuttingDown())
    {
        LOGDEBUG << "request rejected due to shutdown";
        err = ERR_SHUTTING_DOWN;
    }
    else
    {
        // XXX We have to wait for locks to be acquired do the following, should we wait?
        // If this channel has threadpool on this we do not block though.
        // A real nonblocking could install callback upon read-lock acquiring...
        // and continue on next step...
        err = amProcessor->modifyVolumePolicy(vdesc);
    }

    hdr->msg_code = err.GetErrno();
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyVolMod), *vol_msg);
}

// AddVol
// ---------
//
void
AMSvcHandler::AddVol(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                        boost::shared_ptr<fpi::CtrlNotifyVolAdd> &vol_msg)
{
    // We do nothing here...we really shouldn't get these messages at all
    Error err(ERR_OK);
    hdr->msg_code = err.GetErrno();
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyVolAdd), *vol_msg);
}

// RemoveVol
// ---------
//
void
AMSvcHandler::RemoveVol(boost::shared_ptr<fpi::AsyncHdr>            &hdr,
                        boost::shared_ptr<fpi::CtrlNotifyVolRemove> &vol_msg)
{
    Error err(ERR_OK);

    fds_volid_t vol_uuid (vol_msg->vol_desc.volUUID);
    GLOGNOTIFY << "volid:" << vol_uuid << " received volume detach event from OM";

    if (amProcessor->isShuttingDown())
    {
        LOGDEBUG << "request rejected due to shutdown";
        err = ERR_SHUTTING_DOWN;
    }
    else
    {
        amProcessor->removeVolume(vol_msg->vol_desc);
    }

    hdr->msg_code = err.GetErrno();
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyVolRemove), *vol_msg);
}

void
AMSvcHandler::NotifyDMTUpdate(boost::shared_ptr<fpi::AsyncHdr>            &hdr,
                              boost::shared_ptr<fpi::CtrlNotifyDMTUpdate> &msg)
{
    Error err(ERR_OK);

    LOGNOTIFY << "code:" <<  hdr->msg_code << " OMClient received notify DMT update";

    if (amProcessor->isShuttingDown())
    {
        LOGDEBUG << "request rejected due to shutdown";
        err = ERR_SHUTTING_DOWN;
    }
    else
    {
        err = amProcessor->updateDmt(msg->dmt_data.dmt_type,
                                     msg->dmt_data.dmt_data,
                                     std::bind(&AMSvcHandler::NotifyDMTUpdateCb,
                                               this,
                                               hdr,
                                               msg,
                                               std::placeholders::_1));
    }

    // send response right away on error or if there is no IO pending for
    // the previous DMT
    if (err != ERR_IO_PENDING) {
        NotifyDMTUpdateCb(hdr, msg, err);
    }
    // else we will get a callback from DLT manager when there are no more
    // IO pending for the previous DLT, and then we will send response
}

// NotifyDMTUpdateCb
//
//
void
AMSvcHandler::NotifyDMTUpdateCb(boost::shared_ptr<fpi::AsyncHdr>            hdr,
                                boost::shared_ptr<fpi::CtrlNotifyDMTUpdate> msg,
                                const Error                                 &err) {
    LOGDEBUG << "err:" << err << " version:" << msg->dmt_version << " sending response for DMT";
    hdr->msg_code = err.GetErrno();
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyDMTUpdate), *msg);
}

// NotifyDLTUpdate
// ---------------
//
void
AMSvcHandler::NotifyDLTUpdate(boost::shared_ptr<fpi::AsyncHdr>            &hdr,
                              boost::shared_ptr<fpi::CtrlNotifyDLTUpdate> &dlt)
{
    Error err(ERR_OK);

    LOGNOTIFY << "type:" << dlt->dlt_data.dlt_type << " OMClient received new DLT commit";

    if (amProcessor->isShuttingDown())
    {
        LOGDEBUG << "request rejected due to shutdown";
        err = ERR_SHUTTING_DOWN;
    }
    else
    {
        err = amProcessor->updateDlt(dlt->dlt_data.dlt_type,
                                                 dlt->dlt_data.dlt_data,
                                                 std::bind(&AMSvcHandler::NotifyDLTUpdateCb,
                                                           this,
                                                           hdr,
                                                           dlt,
                                                           std::placeholders::_1));
    }

    // send response right away on error or if there is no IO pending for
    // the previous DLT
    if (err != ERR_IO_PENDING) {
        NotifyDLTUpdateCb(hdr, dlt, err);
    }
    // else we will get a callback from DLT manager when there are no more
    // IO pending for the previous DLT, and then we will send response
}

void
AMSvcHandler::NotifyDLTUpdateCb(boost::shared_ptr<fpi::AsyncHdr>            hdr,
                                boost::shared_ptr<fpi::CtrlNotifyDLTUpdate> dlt,
                                const Error                                 &err) {
    LOGDEBUG << "err:" << err << " type:" << dlt->dlt_data.dlt_type << " sending response for DLT";
    hdr->msg_code = static_cast<int32_t>(err.GetErrno());
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyDLTUpdate), *dlt);
}

/**
 * Send the response back to OM for PrepareForShutdown Message.
 */
void
AMSvcHandler::prepareForShutdownMsgRespCb(boost::shared_ptr<fpi::AsyncHdr>  &hdr,
                                          boost::shared_ptr<fpi::PrepareForShutdownMsg> &shutdownMsg)
{
    LOGNOTIFY << "sending PrepareForShutdownMsg response back to OM";
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::PrepareForShutdownMsg), *shutdownMsg);
    /**
     * Delete reference to the amProcessor as we are in shutdown sequence.
     */
    amProcessor.reset();
}

/**
 * Initiate cleanup in preparation for shutdown
 */
void
AMSvcHandler::shutdownAM(boost::shared_ptr<fpi::AsyncHdr>           &hdr,
                         boost::shared_ptr<fpi::PrepareForShutdownMsg>     &shutdownMsg)
{
    Error err(ERR_OK);

    LOGNOTIFY << "AM shutting down";

    /*
     * Bind the callback for PrepareForShutdown Message. This will be called
     * once the data servers are stopped.
     */
     amProcessor->prepareForShutdownMsgRespBindCb(std::bind(
                                                    &AMSvcHandler::prepareForShutdownMsgRespCb,
                                                    this,
                                                    hdr,
                                                    shutdownMsg));

     /**
      * Block any more requests.
      * Drain queues and allow outstanding requests to complete.
      */
     g_fdsprocess->stop();

     /*
      * It's an async shutdown as we cleanup. So acknowledge the message now.
      */
     hdr->msg_code = err.GetErrno();
}

void
AMSvcHandler::addToVolumeGroup(fpi::AsyncHdrPtr& asyncHdr,
                               fpi::AddToVolumeGroupCtrlMsgPtr& addMsg)
{
    amProcessor->addToVolumeGroup(
        addMsg,
        [this, asyncHdr](const Error& e,
                         const fpi::AddToVolumeGroupRespCtrlMsgPtr &payload) {
        asyncHdr->msg_code = e.GetErrno();
        sendAsyncResp(*asyncHdr,
                      FDSP_MSG_TYPEID(fpi::AddToVolumeGroupRespCtrlMsg),
                      *payload);
        });
}

/**
 * Handler to handle switching of coordinator from another AM.
 * If we can't look up the volume we immediately reply.
 */
void
AMSvcHandler::switchCoordinator(boost::shared_ptr<fpi::AsyncHdr>&           hdr,
                                fpi::SwitchCoordinatorMsgPtr&               msg)
{
    auto vol = amProcessor->getVolume(static_cast<fds_volid_t>(msg->volumeId));
    if (nullptr == vol) {
        LOGDEBUG << "volid:" << msg->volumeId << " unable to find volume";
        completeFlush(hdr, ERR_OK);
    } else {
        LOGDEBUG << "vol:" << vol->name << " switching coordinator";
        addPendingFlush(vol->name, hdr);
    }
}

/**
 * Track which AMs are waiting for a volume to flush.
 * If a flush is already happening we just add it to the
 * pending list. If this is the first flush for a volume
 * we trigger the flush and pass it a DetachVolumeReq to queue
 * up when it's done, which will call our callback upon completion.
 * This ensures that all AMs will be responded to, but that we only
 * have one flush in transit.
 */
void
AMSvcHandler::addPendingFlush(std::string const&                  volName,
                              boost::shared_ptr<fpi::AsyncHdr>&   hdr)
{
    std::lock_guard<std::mutex> l(_flush_map_lock);
    auto it = _pendingFlushes.end();
    bool happened {false};
    std::tie(it, happened) = _pendingFlushes.emplace(volName, nullptr);
    if (happened) {
        it->second.reset(new std::set<boost::shared_ptr<fpi::AsyncHdr>>());
    }
    it->second->emplace(hdr);
    if (happened) {
        if (amProcessor->isShuttingDown())
        {
            LOGDEBUG << "request ignored due to shutdown already in progress";
            flushCb(volName, ERR_OK);
        } else {
            // Closure for response call
            auto closure = [this, volName] (DetachCallback* cb, fpi::ErrorCode const& e) mutable -> void {
                _flushCb(volName, e);
            };

            auto callback = create_async_handler<DetachCallback>(std::move(closure));

            AmRequest *volReq = new DetachVolumeReq(invalid_vol_id, volName, callback);
            amProcessor->flushVolume(volReq, volName);
        }
    }
}

/**
 * Callback that will be called when the DetachVolumeReq completes
 * after flushing a volume. We will reply to all AMs waiting for this
 * volume to be flushed.
 * This call is not thread safe and if the _flush_map_lock isn't
 * already being held, then _flushCb should be called instead.
 */
void
AMSvcHandler::flushCb(std::string const& volName, Error const& err) {
    auto it = _pendingFlushes.find(volName);
    if (_pendingFlushes.end() != it) {
        LOGDEBUG << "vol:" << volName << " completing flush of volume";
        for (auto& hdr : *(it->second)) {
            completeFlush(hdr, err);
        }
        _pendingFlushes.erase(it);
    } else {
        LOGERROR << "vol:" << volName << " unable to find pending flush";
    }
}

/**
 * Should be called when lock isn't already held, such as the callback case.
 */
void
AMSvcHandler::_flushCb(std::string const& volName, Error const& err) {
    std::lock_guard<std::mutex> l(_flush_map_lock);
    flushCb(volName, err);
}

/**
 * Reply to AM when switching of coordinator is complete.
 */
void
AMSvcHandler::completeFlush(boost::shared_ptr<fpi::AsyncHdr> const& hdr, Error const& err) {
    hdr->msg_code = err.GetErrno();
    fpi::SwitchCoordinatorRespMsg vol_msg{};
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::SwitchCoordinatorRespMsg), vol_msg);
}


}  // namespace fds
