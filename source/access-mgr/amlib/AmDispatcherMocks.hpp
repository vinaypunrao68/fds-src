/* Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_AMLIB_INCLUDE_AMDISPATCHERMOCKS_H
#define SOURCE_AMLIB_INCLUDE_AMDISPATCHERMOCKS_H

namespace fds {
struct AmDispatcherMockCbs
{
    static void getObjectCb(AmRequest *amReq)
    {
    }
    static void queryCatalogCb(AmRequest *amReq)
    {
        auto blobReq = static_cast<GetBlobReq*>(amReq);
        if (blobReq->get_metadata) {
            auto cb = std::dynamic_pointer_cast<GetObjectWithMetadataCallback>(amReq->cb);
            cb->blobDesc = boost::make_shared<BlobDescriptor>();
            cb->blobDesc->setBlobSize(amReq->data_len);
        }
        blobReq->object_ids.clear();
        blobReq->object_ids.emplace_back(new ObjectID());
    }
};
}  // namespace fds

#endif  // SOURCE_AMLIB_INCLUDE_AMDISPATCHERMOCKS_H
