/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <fdsp_utils.h>
#include <ObjectId.h>
#include <apis/ConfigurationService.h>
#include <DataGen.hpp>
#include <boost/make_shared.hpp>
#include <SvcMsgFactory.h>

namespace fds {

fpi::PutObjectMsgPtr
SvcMsgFactory::newPutObjectMsg(const uint64_t& volId, DataGenIfPtr dataGen)
{
    auto data = dataGen->nextItem();
    auto objId = ObjIdGen::genObjectId(data->c_str(), data->length());
    fpi::PutObjectMsgPtr putObjMsg(new fpi::PutObjectMsg);
    putObjMsg->volume_id = volId;
    putObjMsg->origin_timestamp = fds::util::getTimeStampMillis();
    putObjMsg->data_obj = *data;
    putObjMsg->data_obj_len = data->length();
    fds::assign(putObjMsg->data_obj_id, objId);
    return putObjMsg;
}

fpi::GetObjectMsgPtr
SvcMsgFactory::newGetObjectMsg(const uint64_t& volId, const ObjectID& objId)
{
    fpi::GetObjectMsgPtr getObjMsg = boost::make_shared<fpi::GetObjectMsg>();
    getObjMsg->volume_id = volId;
    assign(getObjMsg->data_obj_id, objId);
    return getObjMsg;
}

apis::VolumeSettings SvcMsgFactory::defaultS3VolSettings()
{
    apis::VolumeSettings vs;
    vs.maxObjectSizeInBytes = 2 << 20;
    vs.volumeType = apis::OBJECT;
    return vs;
}

}  // namespace fds
