/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_SVCMSGFACTORY_H_
#define SOURCE_INCLUDE_SVCMSGFACTORY_H_

#include <boost/shared_ptr.hpp>
#include <fds_types.h>

/* Forward declarations */
namespace FDS_ProtocolInterface {
class PutObjectMsg;
typedef boost::shared_ptr<PutObjectMsg> PutObjectMsgPtr;
class GetObjectMsg;
typedef boost::shared_ptr<GetObjectMsg> GetObjectMsgPtr;
}

namespace fds {
/* Forward declarations */
class DataGenIf;
typedef boost::shared_ptr<DataGenIf> DataGenIfPtr;

namespace apis {
class VolumeSettings;
}

/**
* @brief Factory for generating service messages as well as useful fdsp/xdi objects
*/
struct SvcMsgFactory {
    static FDS_ProtocolInterface::PutObjectMsgPtr
        newPutObjectMsg(const uint64_t& volId, DataGenIfPtr dataGen);

    static FDS_ProtocolInterface::GetObjectMsgPtr
        newGetObjectMsg(const uint64_t& volId, const ObjectID& objId);

    static apis::VolumeSettings defaultS3VolSettings();
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_SVCMSGFACTORY_H_
