/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <StorMgr.h>
#include <net/net-service-tmpl.hpp>
#include <SMSvcHandler.h>

namespace fds {

extern ObjectStorMgr *objStorMgr;

SMSvcHandler::SMSvcHandler()
{
}

void SMSvcHandler::getObject(boost::shared_ptr<fpi::GetObjectMsg>& getObjMsg)  // NOLINT
{
    Error err(ERR_OK);
    auto read_data_msg = new SmIoReadObjectdata();
    read_data_msg->io_type = FDS_SM_READ_OBJECTDATA;
    read_data_msg->obj_data.obj_id = getObjMsg->data_obj_id;
    read_data_msg->smio_readdata_resp_cb = std::bind(
            &SMSvcHandler::getObjectCb, this, getObjMsg,
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
    auto resp = boost::make_shared<GetObjectResp>();
    resp->hdr.msg_code = static_cast<int32_t>(err.GetErrno());
    resp->data_obj_len = read_data->obj_data.data.size();
    resp->data_obj = read_data->obj_data.data;
    NetMgr::ep_mgr_singleton()->ep_send_async_resp(getObjMsg->hdr, *resp);

    delete read_data;
}

}  // namespace fds
