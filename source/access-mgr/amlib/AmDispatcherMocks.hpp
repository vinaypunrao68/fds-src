/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_AMLIB_INCLUDE_AMDISPATCHERMOCKS_H
#define SOURCE_AMLIB_INCLUDE_AMDISPATCHERMOCKS_H

namespace fds {
struct AmDispatcherMockCbs
{
    static void getObjectCb(AmRequest *amReq)
    {
        GetObjectCallback::ptr cb = SHARED_DYN_CAST(GetObjectCallback, amReq->cb);
        cb->returnSize = amReq->data_len;
        cb->returnBuffer = new char[cb->returnSize];   
        memset(cb->returnBuffer, 0x00, cb->returnSize);
        amReq->proc_cb(ERR_OK);                        
    }
    static void queryCatalogCb(AmRequest *amReq)
    {
        GetObjectCallback::ptr cb = SHARED_DYN_CAST(GetObjectCallback, amReq->cb);
        cb->blobDesc.setBlobSize(cb->returnSize);
        amReq->obj_id = ObjectID();
        amReq->proc_cb(ERR_OK);
    }
};
}  // namespace fds

#endif  // SOURCE_AMLIB_INCLUDE_AMDISPATCHERMOCKS_H
