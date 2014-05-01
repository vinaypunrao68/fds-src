/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <convert.h>

namespace fds {
namespace convert {

void getFDSPCreateVolRequest(fpi::FDSP_MsgHdrTypePtr& header,
                             fpi::FDSP_CreateVolTypePtr& request,
                             const std::string& domain,
                             const std::string& volume,
                             const apis::VolumePolicy volPolicy) {

    header.reset(new fpi::FDSP_MsgHdrType());
    request.reset(new fpi::FDSP_CreateVolType());

    header->msg_code = fpi::FDSP_MSG_CREATE_VOL;
    header->bucket_name = volume;
    header->local_domain_id = 1;
    request->vol_name = volume;

    request->vol_info.vol_name = volume;
    request->vol_info.localDomainId = 1;

    request->vol_info.tennantId = 0;
    request->vol_info.localDomainId = 0;
    request->vol_info.globDomainId = 0;

    // Volume capacity is in MB
    request->vol_info.capacity = (1024*10);  // for now presetting to 10GB
    request->vol_info.maxQuota = 0;
    request->vol_info.volType = fpi::FDSP_VOL_S3_TYPE;

    request->vol_info.defReplicaCnt = 0;
    request->vol_info.defWriteQuorum = 0;
    request->vol_info.defReadQuorum = 0;
    request->vol_info.defConsisProtocol = fpi::FDSP_CONS_PROTO_STRONG;

    request->vol_info.volPolicyId = 50;  // default S3 policy desc ID
    request->vol_info.archivePolicyId = 0;
    request->vol_info.placementPolicy = 0;
    request->vol_info.appWorkload = fpi::FDSP_APP_WKLD_TRANSACTION;
    request->vol_info.mediaPolicy = fpi::FDSP_MEDIA_POLICY_HDD;
}

void getFDSPDeleteVolRequest(fpi::FDSP_MsgHdrTypePtr& header,
                             fpi::FDSP_DeleteVolTypePtr& request,
                             const std::string& domain,
                             const std::string& volume) {
    header.reset(new fpi::FDSP_MsgHdrType());
    request.reset(new fpi::FDSP_DeleteVolType());

    header->msg_code = fpi::FDSP_MSG_DELETE_VOL;
    header->bucket_name = volume;
    header->local_domain_id = 1;

    request->vol_name = volume;
    request->domain_id = 1;
}

}  // namespace convert
}  // namespace fds
