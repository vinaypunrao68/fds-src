/*
 * Copyright 2014 Formation Data Systems, Inc.
*/
#include <MockSMCallbacks.h>
#include <string>

namespace fds {

void MockSMCallbacks::mockPutCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr)
{
    auto resp = boost::make_shared<fpi::PutObjectRspMsg>();
    asyncHdr->msg_code = ERR_OK;
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::PutObjectRspMsg), *resp);
}

void MockSMCallbacks::mockGetCb(boost::shared_ptr<fpi::AsyncHdr>& asyncHdr)
{
    auto resp = boost::make_shared<fpi::GetObjectResp>();
    resp->data_obj = std::move(std::string(4096, 'a'));
    asyncHdr->msg_code = ERR_OK;
    sendAsyncResp(*asyncHdr, FDSP_MSG_TYPEID(fpi::GetObjectResp), *resp);
}
}  // namespace fds
