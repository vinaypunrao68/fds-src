/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OMUTILS_H_
#define SOURCE_ORCH_MGR_INCLUDE_OMUTILS_H_
#include <string>
#include <fds_types.h>
namespace fds {
/**
 * This is case insensitive
 */
fds_uint64_t getUuidFromVolumeName(const std::string& name);
fds_uint64_t getUuidFromResourceName(const std::string& name);

}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_OMUTILS_H_
