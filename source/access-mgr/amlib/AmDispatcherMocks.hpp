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
        cb->returnBuffer = boost::make_shared<std::string>(cb->returnSize, 0x00);
        amReq->proc_cb(ERR_OK);                        
    }
    static void queryCatalogCb(AmRequest *amReq)
    {
        if (static_cast<GetBlobReq*>(amReq)->get_metadata) {
            auto cb = SHARED_DYN_CAST(GetObjectWithMetadataCallback, amReq->cb);
            cb->blobDesc = boost::make_shared<BlobDescriptor>();
            cb->blobDesc->setBlobSize(cb->returnSize);
        }
        amReq->obj_id = ObjectID();
        amReq->proc_cb(ERR_OK);
    }
};
}  // namespace fds

#endif  // SOURCE_AMLIB_INCLUDE_AMDISPATCHERMOCKS_H
