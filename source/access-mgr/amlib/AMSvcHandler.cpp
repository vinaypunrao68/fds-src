/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */

#include <AMSvcHandler.h>

#include <fdsp/dm_api_types.h>
#include "net/SvcRequest.h"
#include "AmProcessor.h"

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

    LOGNORMAL << "qos ctrl set total rate " << msg->qosctrl.total_rate;

    if (amProcessor->isShuttingDown())
    {
        LOGDEBUG << "Request rejected due to shutdown in progress.";
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

    LOGNORMAL << " set throttle as " << msg->throttle.throttle_level;

    if (amProcessor->isShuttingDown()) {
        LOGDEBUG << "Request rejected due to shutdown in progress.";
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
    VolumeDesc vdesc(vol_msg->vol_desc), * vdb = &vdesc;
    GLOGNOTIFY << "Received volume modify  event from OM"
               << " for volume " << vdb->name << ":" << vol_uuid;

    if (amProcessor->isShuttingDown())
    {
        LOGDEBUG << "Request rejected due to shutdown in progress.";
        err = ERR_SHUTTING_DOWN;
    }
    else
    {
        // XXX We have to wait for locks to be acquired do the following, should we wait?
        // If this channel has threadpool on this we do not block though.
        // A real nonblocking could install callback upon read-lock acquiring...
        // and continue on next step...
        err = amProcessor->modifyVolumePolicy(vol_uuid, vdesc);
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
    Error err(ERR_OK);

    fds_volid_t vol_uuid (vol_msg->vol_desc.volUUID);
    GLOGNOTIFY << "Received volume add event from OM"
                       << " for volume \"" << vol_msg->vol_desc.vol_name << "\" ["
                       << vol_uuid << "]";

    if (amProcessor->isShuttingDown())
    {
        LOGDEBUG << "Request rejected due to shutdown in progress.";
        err = ERR_SHUTTING_DOWN;
    }
    else
    {
        VolumeDesc vdesc(vol_msg->vol_desc);

        if (vol_uuid != invalid_vol_id) {
            /** Registration is always a success, but the VolumeOpen may fail */
            try {
                amProcessor->registerVolume(vdesc);
            } catch(Exception& e) {
                err = e.getError();
                GLOGWARN << err;
            }
        }
    }

    // we return without wait for that to be clearly finish, not big deal.
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
    GLOGNOTIFY << "Received volume detach event from OM"
               << " for volume " << vol_uuid;

    if (amProcessor->isShuttingDown())
    {
        LOGDEBUG << "Request rejected due to shutdown in progress.";
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

    LOGNOTIFY << "OMClient received notify DMT update " << hdr->msg_code;

    if (amProcessor->isShuttingDown())
    {
        LOGDEBUG << "Request rejected due to shutdown in progress.";
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
    LOGDEBUG << "Sending response for DMT version " << msg->dmt_version << " "  << err;
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

    LOGNOTIFY << "OMClient received new DLT commit version  "
            << dlt->dlt_data.dlt_type;

    if (amProcessor->isShuttingDown())
    {
        LOGDEBUG << "Request rejected due to shutdown in progress.";
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
    LOGDEBUG << "Sending response for DLT version " << dlt->dlt_data.dlt_type
             << " "  << err;
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
    LOGNOTIFY << "Data servers stopped. Sending PrepareForShutdownMsg response back to OM";
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

    LOGNOTIFY << "OMClient received shutdown message ... AM shutting down...";

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
     amProcessor->stop();

     /*
      * It's an async shutdown as we cleanup. So acknowledge the message now.
      */
     hdr->msg_code = err.GetErrno();
}

}  // namespace fds
