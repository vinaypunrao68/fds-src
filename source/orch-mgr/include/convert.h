/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_CONVERT_H_
#define SOURCE_ORCH_MGR_INCLUDE_CONVERT_H_
#include <fdsp/ConfigurationService.h>
#include <fds_typedefs.h>
#include <string>
#include <OmResources.h>
namespace fds { namespace convert {

fpi::FDSP_MediaPolicy getMediaPolicyToFDSP_MediaPolicy( const apis::MediaPolicy mediaPolicy );

void getFDSPCreateVolRequest(fpi::FDSP_MsgHdrTypePtr& header,
                             apis::FDSP_CreateVolTypePtr& request,
                             const std::string& domain,
                             const std::string& volume,
                             const apis::VolumeSettings volSettings);

void getFDSPDeleteVolRequest(fpi::FDSP_MsgHdrTypePtr& header,
                             apis::FDSP_DeleteVolTypePtr& request,
                             const std::string& domain,
                             const std::string& volume);

void getVolumeDescriptor(apis::VolumeDescriptor& volDescriptor, VolumeInfo::pointer  vol);

}  // namespace convert
}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_CONVERT_H_
