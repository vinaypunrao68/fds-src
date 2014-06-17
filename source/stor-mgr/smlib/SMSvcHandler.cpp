/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <StorMgr.h>
#include <net/net-service-tmpl.hpp>
#include <fdsp_utils.h>
#include <fds_assert.h>
#include <SMSvcHandler.h>
#include <platform/fds_flags.h>
#include <sm-platform.h>

namespace fds {

extern ObjectStorMgr *objStorMgr;

SMSvcHandler::SMSvcHandler()
{
}

void SMSvcHandler::getObject(boost::shared_ptr<fpi::GetObjectMsg>& getObjMsg)  // NOLINT
{
    DBG(GLOGDEBUG << fds::logString(*getObjMsg));

    DBG(FLAG_CHECK_RETURN_VOID(common_drop_async_resp > 0));
#if 0
    if (objStorMgr->getDLT()->getPrimary(ObjectID(getObjMsg->data_obj_id.digest)).\
        uuid_get_val() == static_cast<uint64_t>(getObjMsg->hdr.msg_dst_uuid.svc_uuid) ||
        objStorMgr->getDLT()->getNode(ObjectID(getObjMsg->data_obj_id.digest), 1).\
        uuid_get_val() == static_cast<uint64_t>(getObjMsg->hdr.msg_dst_uuid.svc_uuid)) {
        DBG(GLOGDEBUG << "Ignoring");
        return;
    }
#endif

    Error err(ERR_OK);
    auto read_data_msg = new SmIoReadObjectdata();
    read_data_msg->io_type = FDS_SM_READ_OBJECTDATA;
    read_data_msg->setObjId(ObjectID(getObjMsg->data_obj_id.digest));
    read_data_msg->obj_data.obj_id = getObjMsg->data_obj_id;
    read_data_msg->smio_readdata_resp_cb = std::bind(
            &SMSvcHandler::getObjectCb, this,
            getObjMsg,
            std::placeholders::_1, std::placeholders::_2);

    // TODO(Rao): Change the queue to the right volume queue
    err = objStorMgr->enqueueMsg(FdsSysTaskQueueId, read_data_msg);
    if (err != fds::ERR_OK) {
        fds_assert(!"Hit an error in enqueing");
        LOGERROR << "Failed to enqueue to SmIoReadObjectMetadata to StorMgr.  Error: "
                << err;
        getObjectCb(getObjMsg, err, read_data_msg);
    }
}

void SMSvcHandler::getObjectCb(boost::shared_ptr<fpi::GetObjectMsg>& getObjMsg,
                               const Error &err,
                               SmIoReadObjectdata *read_data)
{
    DBG(GLOGDEBUG << fds::logString(*getObjMsg));

    auto resp = boost::make_shared<GetObjectResp>();
    resp->hdr.msg_code = static_cast<int32_t>(err.GetErrno());
    resp->data_obj_len = read_data->obj_data.data.size();
    resp->data_obj = read_data->obj_data.data;
    NetMgr::ep_mgr_singleton()->ep_send_async_resp(getObjMsg->hdr, *resp);

    delete read_data;
}

}  // namespace fds
