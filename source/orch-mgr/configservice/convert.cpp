/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <convert.h>
#include <string>
#include <util/timeutils.h>
namespace fds {
namespace convert {

void getFDSPCreateVolRequest(fpi::FDSP_MsgHdrTypePtr& header,
                             apis::FDSP_CreateVolTypePtr& request,
                             const std::string& domain,
                             const std::string& volume,
                             const apis::VolumeSettings volSettings) {
    header.reset(new fpi::FDSP_MsgHdrType());
    request.reset(new apis::FDSP_CreateVolType());

    header->msg_code = fpi::FDSP_MSG_CREATE_VOL;
    header->bucket_name = volume;
    header->local_domain_id = 1;
    request->vol_name = volume;

    request->vol_info.vol_name = volume;

    request->vol_info.tennantId = 0;
    request->vol_info.localDomainId = 0;

    // Volume capacity is in MB
    request->vol_info.capacity = volSettings.blockDeviceSizeInBytes / (1024 * 1024);
    request->vol_info.maxObjSizeInBytes =
            volSettings.maxObjectSizeInBytes;
    request->vol_info.contCommitlogRetention = volSettings.contCommitlogRetention;

    // Set connector
    // TODO(Andrew): Have the api service just replace the fdsp version
    // so that this conversion isn't needed
    switch (volSettings.volumeType) {
        case apis::OBJECT:
            request->vol_info.volType = fpi::FDSP_VOL_S3_TYPE;
            break;
        case apis::BLOCK:
            request->vol_info.volType = fpi::FDSP_VOL_BLKDEV_TYPE;
            break;
        default:
            fds_panic("Unknown connector type!");
    }

    // TODO(Andrew): Don't hard code to policy 50
    request->vol_info.volPolicyId = 50;
    request->vol_info.placementPolicy = 0;
    request->vol_info.mediaPolicy = fpi::FDSP_MEDIA_POLICY_HDD;
    request->vol_info.createTime = util::getTimeStampMillis();
}

void getFDSPDeleteVolRequest(fpi::FDSP_MsgHdrTypePtr& header,
                             apis::FDSP_DeleteVolTypePtr& request,
                             const std::string& domain,
                             const std::string& volume) {
    header.reset(new fpi::FDSP_MsgHdrType());
    request.reset(new apis::FDSP_DeleteVolType());

    header->msg_code = fpi::FDSP_MSG_DELETE_VOL;
    header->bucket_name = volume;
    header->local_domain_id = 1;

    request->vol_name = volume;
    request->domain_id = 1;
}

void getVolumeDescriptor(apis::VolumeDescriptor& volDescriptor, VolumeInfo::pointer vol) {
    volDescriptor.name = vol->vol_get_name();
    VolumeDesc* volDesc = vol->vol_get_properties();

    // volume settings
    switch ( volDesc->mediaPolicy )
    {
        case FDS_ProtocolInterface::FDSP_MEDIA_POLICY_UNSET:
          volDescriptor.policy.mediaPolicy = apis::HDD_ONLY;
          break;
        case FDS_ProtocolInterface::FDSP_MEDIA_POLICY_HDD:
          volDescriptor.policy.mediaPolicy = apis::HDD_ONLY;
          break;
        case FDS_ProtocolInterface::FDSP_MEDIA_POLICY_SSD:
          volDescriptor.policy.mediaPolicy = apis::SSD_ONLY;
          break;
        case FDS_ProtocolInterface::FDSP_MEDIA_POLICY_HYBRID:
        case FDS_ProtocolInterface::FDSP_MEDIA_POLICY_HYBRID_PREFCAP:
          volDescriptor.policy.mediaPolicy = apis::HYBRID_ONLY;
          break;
    }

    switch (volDesc->volType) {
        case fpi::FDSP_VOL_BLKDEV_TYPE:
            volDescriptor.policy.blockDeviceSizeInBytes =
              ( volDesc->capacity * ( 1024 * 1024 ) );
            volDescriptor.policy.volumeType = apis::BLOCK;
            break;
        default:
            volDescriptor.policy.volumeType = apis::OBJECT;
    }
    volDescriptor.policy.maxObjectSizeInBytes =
            volDesc->maxObjSizeInBytes;
    volDescriptor.tenantId = volDesc->tennantId;
    volDescriptor.state    = volDesc->getState();
    volDescriptor.volId    = volDesc->volUUID.get();
}

}  // namespace convert
}  // namespace fds
