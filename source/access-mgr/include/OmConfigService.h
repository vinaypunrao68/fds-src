/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ACCESS_MGR_INCLUDE_OMCONFIGSERVICE_H_
#define SOURCE_ACCESS_MGR_INCLUDE_OMCONFIGSERVICE_H_

#include <string>
#include <fds_types.h>
#include <apis/ConfigurationService.h>

namespace fds {

class OmConfigApi : public boost::noncopyable {
  private:
    boost::shared_ptr<apis::ConfigurationServiceClient> omConfigClient;
    std::string omConfigIp;
    fds_uint32_t omConfigPort;

  public:
    OmConfigApi();
    ~OmConfigApi();
    typedef boost::shared_ptr<OmConfigApi> shared_ptr;

    Error statVolume(boost::shared_ptr<std::string> volumeName,
                     apis::VolumeDescriptor &volDesc);
};

}  // namespace fds

#endif  // SOURCE_ACCESS_MGR_INCLUDE_OMCONFIGSERVICE_H_
