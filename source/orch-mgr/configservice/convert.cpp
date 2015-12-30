/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <exception>
#include <convert.h>
#include <string>
#include <util/timeutils.h>
namespace fds {
namespace convert {

fpi::FDSP_MediaPolicy getMediaPolicyToFDSP_MediaPolicy( const apis::MediaPolicy mediaPolicy )
{
    switch ( mediaPolicy )
    {
        case apis::SSD_ONLY:
            return fpi::FDSP_MEDIA_POLICY_SSD;
        case apis::HDD_ONLY:
            return fpi::FDSP_MEDIA_POLICY_HDD;
        case apis::HYBRID_ONLY:
            return fpi::FDSP_MEDIA_POLICY_HYBRID;
    }

    LOGWARN << "Defaulting media policy to "
            << fpi::FDSP_MEDIA_POLICY_HDD
            << ", because of unknown media policy ( "
            << mediaPolicy
            << " ).";

    // default for unknown media policy
    return fpi::FDSP_MEDIA_POLICY_HDD;
}

void getFDSPCreateVolRequest(fpi::FDSP_MsgHdrTypePtr& header,
                             apis::FDSP_CreateVolTypePtr& request,
                             const std::string& domain,
                             const std::string& volume,
                             const apis::VolumeSettings volSettings)
{
    header.reset(new fpi::FDSP_MsgHdrType());
    /*
     * shouldn't this clear/initialize to well known values?
     */
    request.reset(new apis::FDSP_CreateVolType());

    header->msg_code = fpi::FDSP_MSG_CREATE_VOL;
    header->bucket_name = volume;
    header->local_domain_id = 1;

    /*
     * FDSP_VolumeDescType:
     *   1: required string            vol_name
     *   2: i32                        tennantId
     *   3: i32                        localDomainId
     *   4: required i64               volUUID,
     *
     *   // Basic operational properties
     *   5: required FDSP_VolType      volType,
     *   6: i32                        maxObjSizeInBytes,
     *   7: required double            capacity,
     *
     *   // Other policies
     *   8: i32                        volPolicyId,
     *   9: i32                        placementPolicy,
     *
     *   // volume policy details
     *   10: i64                       iops_assured,
     *   11: i64                       iops_throttle,
     *   12: i32                       rel_prio,
     *   13: required FDSP_MediaPolicy mediaPolicy,
     *
     *   14: bool                      fSnapshot,
     *   15: ResourceState             state,
     *   16: i64                       contCommitlogRetention,
     *   17: i64                       srcVolumeId,
     *   18: i64                       timelineTime,
     *   19: i64                       createTime,
     *   20: common.IScsiTarget        iscsi,
     *   21: common.NfsOption          nfs
     */

    request->vol_name = volume;

    request->vol_info.tennantId = 0;
    request->vol_info.localDomainId = 0;
    request->vol_info.vol_name = volume;

    /*
     * attribute volUUID will be set when the volume is created.
     */

    switch ( volSettings.volumeType )
    {
        case apis::OBJECT:
            LOGDEBUG << "Found OBJECT volume Type";
            request->vol_info.volType = fpi::FDSP_VOL_S3_TYPE;
            break;
        case apis::NFS: // extended from Object settings
            LOGDEBUG << "Found NFS volume Type";
            request->vol_info.volType = fpi::FDSP_VOL_NFS_TYPE;
            request->vol_info.nfs = volSettings.nfsOptions;
            LOGDEBUG << "NFS:: [ " << request->vol_info.nfs.client << " ] "
                     << "[ " << request->vol_info.nfs.options << " ]";
            break;
        case apis::BLOCK:
            LOGDEBUG << "Found BLOCK volume Type";
            request->vol_info.volType = fpi::FDSP_VOL_BLKDEV_TYPE;
            // Volume capacity is in MB
            break;
        case apis::ISCSI: // extended from Block settings
            LOGDEBUG << "Found iSCSI volume Type";
            request->vol_info.volType = fpi::FDSP_VOL_ISCSI_TYPE;
            request->vol_info.iscsi = volSettings.iscsiTarget;
            LOGDEBUG << "LUN count [ " << request->vol_info.iscsi.luns.size() << " ]";
            for ( auto lun : request->vol_info.iscsi.luns )
            {
                LOGDEBUG << "name [ " << lun.name << " ] access [ " << lun.access << " ]";
            }

            LOGDEBUG << "Initiator count [ " << request->vol_info.iscsi.initiators.size() << " ]";
            for ( auto initiator : request->vol_info.iscsi.initiators )
            {
                LOGDEBUG << "wwn mask [ " << initiator.wwn_mask << " ]";
            }

            LOGDEBUG << "Incoming Users count [ " << request->vol_info.iscsi.incomingUsers.size() << " ]";
            for ( auto credentials : request->vol_info.iscsi.incomingUsers )
            {
                LOGDEBUG << "incoming user [ " << credentials.name << " ] password [ ****** ]";
            }

            LOGDEBUG << "Outgoing Users count [ " << request->vol_info.iscsi.outgoingUsers.size() << " ]";
            for ( auto credentials : request->vol_info.iscsi.outgoingUsers )
            {
                LOGDEBUG << "outgoing user [ " << credentials.name << " ] password [ ****** ]";
            }
            break;
        default:
            std::stringstream errMsg;
            errMsg << "Unsupported Connector Type ( " << volSettings.volumeType << " ).";

            LOGERROR << errMsg;
            throw std::runtime_error( errMsg.str().c_str() );
    }

    /*
     * common to volume settings
     */

    /*
     * TODO(Tinius) it does not make any sense, why do we require this attribute for all volume types.
     * Should only be for OBJECT/NFS based volumes.
     */
    request->vol_info.maxObjSizeInBytes = volSettings.maxObjectSizeInBytes;
    /*
     * TODO(Tinius) it does not make any sense, why do we require this attribute for all volume types.
     * Should only be for BLOCK/iSCSI based volumes.
     */
    request->vol_info.capacity = volSettings.blockDeviceSizeInBytes / ( 1024 * 1024 );

    /*
     * the following common volume settings are defaulted, because no values are passed in.
     *
     * These will be updated later during the volume creation work flow.
     */
    request->vol_info.volPolicyId = 50;
    request->vol_info.placementPolicy = 0;

    request->vol_info.contCommitlogRetention = volSettings.contCommitlogRetention;
    request->vol_info.mediaPolicy = getMediaPolicyToFDSP_MediaPolicy( volSettings.mediaPolicy );
    request->vol_info.createTime = util::getTimeStampSeconds();
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

void getVolumeDescriptor(apis::VolumeDescriptor& volDescriptor, VolumeInfo::pointer vol)
{
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

    switch ( volDesc->volType )
    {
        case fpi::FDSP_VOL_BLKDEV_TYPE:
            LOGDEBUG << "BLOCK volume found [ " << volDescriptor.name << " ]";
            volDescriptor.policy.volumeType = apis::BLOCK;
            volDescriptor.policy.blockDeviceSizeInBytes = ( int64_t ) ( volDesc->capacity * ( 1024 * 1024 ) );
            break;
        case fpi::FDSP_VOL_ISCSI_TYPE:
            LOGDEBUG << "iSCSI volume found [ " << volDescriptor.name << " ]";
            volDescriptor.policy.volumeType = apis::ISCSI;
            volDescriptor.policy.blockDeviceSizeInBytes = ( int64_t ) ( volDesc->capacity * ( 1024 * 1024 ) );
            volDescriptor.policy.iscsiTarget = volDesc->iscsiSettings;

            LOGDEBUG << "iSCSI:: LUN count [ " << iscsi.luns.size() << " ]"
                     << " Initiator count [ " << iscsi.initiators.size() << " ]"
                     << " Incoming Users count [ " << iscsi.incomingUsers.size() << " ]"
                     << " Outgoing Users count [ " << iscsi.outgoingUsers.size() << " ]";
            break;
        case fpi::FDSP_VOL_S3_TYPE:
            LOGDEBUG << "OBJECT volume found [ " << volDescriptor.name << " ]";
            volDescriptor.policy.volumeType = apis::OBJECT;
            volDescriptor.policy.maxObjectSizeInBytes = volDesc->maxObjSizeInBytes;
            break;
        case fpi::FDSP_VOL_NFS_TYPE:
            LOGDEBUG << "NFS volume found [ " << volDescriptor.name << " ]";
            volDescriptor.policy.volumeType = apis::NFS;
            volDescriptor.policy.maxObjectSizeInBytes = volDesc->maxObjSizeInBytes;
            volDescriptor.policy.nfsOptions = volDesc->nfsSettings;
            
            LOGDEBUG << "NFS:: [ " << volDescriptor.policy.nfsOptions.client << " ] "
                     << "[ " << volDescriptor.policy.nfsOptions.options << " ]";
            break;
        default:
            LOGWARN << "Unsupported volume type " << volDesc->volType;
            break;
    }

    volDescriptor.tenantId = volDesc->tennantId;
    volDescriptor.state    = volDesc->getState();
    volDescriptor.volId    = volDesc->volUUID.get();
}

}  // namespace convert
}  // namespace fds
