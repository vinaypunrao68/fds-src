/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#include <net/BaseAsyncSvcHandler.h>

#include <AMSvcHandler.h>

#include "AmCache.h"
#include "StorHvCtrl.h"
#include "StorHvQosCtrl.h"
#include "StorHvVolumes.h"

extern StorHvCtrl * storHvisor;

namespace fds {

extern StorHvQosCtrl *storHvQosCtrl;

AccessMgr::unique_ptr am;

AMSvcHandler::~AMSvcHandler() {}
AMSvcHandler::AMSvcHandler()
{
    REGISTER_FDSP_MSG_HANDLER(fpi::NodeSvcInfo, notifySvcChange);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyBucketStat, NotifyBucketStats);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyThrottle, SetThrottleLevel);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyQoSControl, QoSControl);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyVolMod, NotifyModVol);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyVolAdd, AttachVol);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyVolRemove, DetachVol);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyDLTUpdate, NotifyDLTUpdate);
    REGISTER_FDSP_MSG_HANDLER(fpi::CtrlNotifyDMTUpdate, NotifyDMTUpdate);
    REGISTER_FDSP_MSG_HANDLER(fpi::ShutdownMODMsg, shutdownAM);
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

    if (am->isShuttingDown())
    {
        LOGDEBUG << "Request rejected due to shutdown in progress.";
        err = ERR_SHUTTING_DOWN;
    }
    else
    {
        err = storHvQosCtrl->htb_dispatcher->modifyTotalRate(msg->qosctrl.total_rate);
    }

    hdr->msg_code = err.GetErrno();
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyQoSControl), *msg);
}

// NotifyBucketStats
// -----------------
//
void
AMSvcHandler::NotifyBucketStats(boost::shared_ptr<fpi::AsyncHdr>             &hdr,
                                boost::shared_ptr<fpi::CtrlNotifyBucketStat> &msg)
{
#if 0
    //  OM receives some bucket stats from somewhere
    FDSP_BucketStatsRespTypePtr buck_stats =
        FDSP_BucketStatsRespTypePtr(new FDSP_BucketStatsRespType(msg->bucket_stat));
    // storHvisor->getBucketStatsResp(rx_msg, buck_stats);  //  postpone need transid
#endif
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

    if (am->isShuttingDown())
    {
        LOGDEBUG << "Request rejected due to shutdown in progress.";
        err = ERR_SHUTTING_DOWN;
    }
    else
    {
        // XXX ignore domain_id right now?
        float  throttle_level = msg->throttle.throttle_level;
        storHvQosCtrl->htb_dispatcher->setThrottleLevel(throttle_level);
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

    auto vol_uuid = vol_msg->vol_desc.volUUID;
    VolumeDesc vdesc(vol_msg->vol_desc), * vdb = &vdesc;
    GLOGNOTIFY << "StorHvVolumeTable - Received volume modify  event from OM"
               << " for volume " << vdb->name << ":" << std::hex
               << vol_uuid << std::dec;

    if (am->isShuttingDown())
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
        err = storHvisor->vol_table->modifyVolumePolicy(vol_uuid, vdesc);
    }

    hdr->msg_code = err.GetErrno();
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyVolMod), *vol_msg);
}

// AttachVol
// ---------
//
void
AMSvcHandler::AttachVol(boost::shared_ptr<fpi::AsyncHdr>         &hdr,
                        boost::shared_ptr<fpi::CtrlNotifyVolAdd> &vol_msg)
{
    Error err(ERR_OK);

    auto vol_uuid = vol_msg->vol_desc.volUUID;
    GLOGNOTIFY << "StorHvVolumeTable - Received volume attach event from OM"
                       << " for volume " << std::hex << vol_uuid << std::dec;

    if (am->isShuttingDown())
    {
        LOGDEBUG << "Request rejected due to shutdown in progress.";
        err = ERR_SHUTTING_DOWN;
    }
    else
    {
        VolumeDesc vdesc(vol_msg->vol_desc);

        if (vol_uuid != invalid_vol_id) {
            err = storHvisor->vol_table->registerVolume(vdesc);
            // TODO(Anna) remove this assert when we implement response handling in AM
            // for crete bucket, if err not ok, it is most likely QOS admission control issue
            fds_verify(err.ok());
            // Create cache structures for volume
            err = storHvisor->amCache->createCache(vdesc);
            fds_verify(err == ERR_OK);
        } else {
            /* complete all requests that are waiting on bucket to attach with error */
            GLOGNOTIFY << "Requested volume "
                       << vdesc.name << " does not exist";
            storHvisor->vol_table->
                    moveWaitBlobsToQosQueue(vol_uuid,
                                            vdesc.name,
                                            ERR_NOT_FOUND);
        }
    }

    // we return without wait for that to be clearly finish, not big deal.
    hdr->msg_code = err.GetErrno();
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyVolAdd), *vol_msg);
}

// DetachVol
// ---------
//
void
AMSvcHandler::DetachVol(boost::shared_ptr<fpi::AsyncHdr>            &hdr,
                        boost::shared_ptr<fpi::CtrlNotifyVolRemove> &vol_msg)
{
    Error err(ERR_OK);

    auto vol_uuid = vol_msg->vol_desc.volUUID;
    GLOGNOTIFY << "StorHvVolumeTable - Received volume detach event from OM"
               << " for volume " << std::hex << vol_uuid << std::dec;

    if (am->isShuttingDown())
    {
        LOGDEBUG << "Request rejected due to shutdown in progress.";
        err = ERR_SHUTTING_DOWN;
    }
    else
    {
        err = storHvisor->vol_table->removeVolume(vol_uuid);
    }

    hdr->msg_code = err.GetErrno();
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyVolRemove), *vol_msg);
}

void
AMSvcHandler::NotifyDMTUpdate(boost::shared_ptr<fpi::AsyncHdr>            &hdr,
                              boost::shared_ptr<fpi::CtrlNotifyDMTUpdate> &msg)
{
    Error err = storHvisor->om_client->updateDmt(msg->dmt_data.dmt_type, msg->dmt_data.dmt_data);
    Error err(ERR_OK);

    LOGNOTIFY << "OMClient received notify DMT update " << hdr->msg_code;

    if (am->isShuttingDown())
    {
        LOGDEBUG << "Request rejected due to shutdown in progress.";
        err = ERR_SHUTTING_DOWN;
    }
    else
    {
        err = storHvisor->om_client->addSerialized(msg->dmt_data.dmt_data, DMT_COMMITTED);
    }

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

    if (am->isShuttingDown())
    {
        LOGDEBUG << "Request rejected due to shutdown in progress.";
        err = ERR_SHUTTING_DOWN;
    }
    else
    {
        err = storHvisor->om_client->updateDlt(dlt->dlt_data.dlt_type, dlt->dlt_data.dlt_data);
    }
    hdr->msg_code = err.GetErrno();
    sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::CtrlNotifyDLTUpdate), *dlt);
}

/**
 * Initiate cleanup in preparation for shutdown
 */
void
AMSvcHandler::shutdownAM(boost::shared_ptr<fpi::AsyncHdr>           &hdr,
                         boost::shared_ptr<fpi::ShutdownMODMsg>     &shutdownMsg)
{
    Error err(ERR_OK);

    LOGNOTIFY << "OMClient received shutdown message ... AM shutting down...";

     /**
      * Block any more requests.
      */
     am->setShutDown();

     /*
      * It's an async shutdown as we cleanup. So acknowledge the message now.
      */
     hdr->msg_code = err.GetErrno();
     sendAsyncResp(*hdr, FDSP_MSG_TYPEID(fpi::ShutdownMODMsg), *shutdownMsg);

     /*
      * Drain queues and allow outstanding requests to complete.
      */
     am->prepareToShutDown();
}

}  // namespace fds
