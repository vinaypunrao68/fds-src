/*
 * Copyright 2016 by Formation Data Systems, Inc.
 * vim: noai:ts=8:sw=2:tw=100:syntax=cpp:et
 */

include "common.thrift"

namespace cpp fds.apis
namespace java com.formationds.apis

struct Version {
  1: i32 major_version;
  2: i32 minor_version;
  3: i32 patch_version;
}

struct ServiceAPIVersion {
  1: string service_family;
  2: string service_name;
  3: string major_alias;
  4: Version api_version;
}

/**
 * Base service for FDS versioned service.
 * Development rules for extensions of this service: 
 *   https://formationds.atlassian.net/wiki/display/ENG/Thrift+API+Versions
 */
service VersionedService {

  Version getVersion(1: i64 nullarg) throws (1: common.ApiException e);

  /**
   * Get the version table for the service. Enables user of client stubs to feed
   * service names to a client stub factory in decreasing version number order.
   */
  list<ServiceAPIVersion> getVersionTable(1: i64 nullarg) throws (1: common.ApiException e);

  /**
   * @param stubVersion: The service version suggested by the client stub
   *
   * @returns Version: The service version acceptable to the endpoint server
   */
  Version suggestVersion(1: required Version stubVersion) throws (1: common.ApiException e);
}
