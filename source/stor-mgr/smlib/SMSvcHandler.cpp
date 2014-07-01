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
    REGISTER_FDSP_MSG_HANDLER(fpi::GetObjectMsg, getObject);
    REGISTER_FDSP_MSG_HANDLER(fpi::PutObjectMsg, putObject);
}

void SMSvcHandler::getObject(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                             boost::shared_ptr<fpi::GetObjectMsg>& getObjMsg)  // NOLINT
{
    DBG(GLOGDEBUG << logString(*asyncHdr) << fds::logString(*getObjMsg));

    DBG(FLAG_CHECK_RETURN_VOID(common_drop_async_resp > 0));
    DBG(FLAG_CHECK_RETURN_VOID(sm_drop_gets > 0));

    Error err(ERR_OK);
    auto read_req = new SmIoReadObjectdata();
    read_req->io_type = FDS_SM_GET_OBJECT;
    read_req->setVolId(getObjMsg->volume_id);
    read_req->setObjId(ObjectID(getObjMsg->data_obj_id.digest));
    read_req->obj_data.obj_id = getObjMsg->data_obj_id;
    read_req->smio_readdata_resp_cb = std::bind(
            &SMSvcHandler::getObjectCb, this,
            asyncHdr,
            std::placeholders::_1, std::placeholders::_2);

    err = objStorMgr->enqueueMsg(read_req->getVolId(), read_req);
    if (err != fds::ERR_OK) {
        fds_assert(!"Hit an error in enqueing");
        LOGERROR << "Failed to enqueue to SmIoReadObjectMetadata to StorMgr.  Error: "
                << err;
        getObjectCb(asyncHdr, err, read_req);
    }
}

void SMSvcHandler::getObjectCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                               const Error &err,
                               SmIoReadObjectdata *read_req)
{
    DBG(GLOGDEBUG << fds::logString(*asyncHdr));

    auto resp = boost::make_shared<GetObjectResp>();
    asyncHdr->msg_code = static_cast<int32_t>(err.GetErrno());
    resp->data_obj_len = read_req->obj_data.data.size();
    resp->data_obj = read_req->obj_data.data;
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::GetObjectResp), *resp);

    delete read_req;
}

void SMSvcHandler::putObject(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                             boost::shared_ptr<fpi::PutObjectMsg>& putObjMsg)  // NOLINT
{
    DBG(GLOGDEBUG << fds::logString(*asyncHdr) << fds::logString(*putObjMsg));

    DBG(FLAG_CHECK_RETURN_VOID(common_drop_async_resp > 0));
    DBG(FLAG_CHECK_RETURN_VOID(sm_drop_puts > 0));

    Error err(ERR_OK);
    auto put_req = new SmIoPutObjectReq();
    put_req->io_type = FDS_SM_PUT_OBJECT;
    put_req->setVolId(putObjMsg->volume_id);
    put_req->origin_timestamp = putObjMsg->origin_timestamp;
    put_req->setObjId(ObjectID(putObjMsg->data_obj_id.digest));
    put_req->data_obj = std::move(putObjMsg->data_obj);
    putObjMsg->data_obj.clear();
    put_req->response_cb= std::bind(
            &SMSvcHandler::putObjectCb, this,
            asyncHdr,
            std::placeholders::_1, std::placeholders::_2);

    err = objStorMgr->enqueueMsg(put_req->getVolId(), put_req);
    if (err != fds::ERR_OK) {
        fds_assert(!"Hit an error in enqueing");
        LOGERROR << "Failed to enqueue to SmIoPutObjectReq to StorMgr.  Error: "
                << err;
        putObjectCb(asyncHdr, err, put_req);
    }
}

void SMSvcHandler::putObjectCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr,
                               const Error &err,
                               SmIoPutObjectReq* put_req)
{
    DBG(GLOGDEBUG << fds::logString(*asyncHdr));

    auto resp = boost::make_shared<fpi::PutObjectRspMsg>();
    asyncHdr->msg_code = static_cast<int32_t>(err.GetErrno());
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::PutObjectRspMsg), *resp);

    delete put_req;
}
}  // namespace fds
