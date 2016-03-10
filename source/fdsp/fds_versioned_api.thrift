/*
 * Copyright 2016 by Formation Data Systems, Inc.
 * vim: noai:ts=8:sw=2:tw=100:syntax=cpp:et
 */

include "common.thrift"

namespace cpp fds.apis
namespace java com.formationds.apis

/**
 * See the Semantic Versioning Specification (SemVer) http://semver.org.
 *
 *  - Major: Incremented for backward incompatible changes. An example would
 *           be changes to the number or disposition of method arguments.
 *  - Minor: Incremented for backward compatible changes. An example would
 *           be the addition of a new (optional) method.
 *  - Patch: Incremented for bug fixes. The patch level should be increased
 *           for every edit that doesn't result in a change to major/minor.
 */
struct Version {
  1: i32 major_version;
  2: i32 minor_version;
  3: i32 patch_version;
}

/**
 * The API version (NOT the product version).
 */
struct ServiceAPIVersion {
  1: string service_family;
  2: string service_name;
  3: string major_alias;
  4: Version api_version;
}

/**
 * @brief Base service for FDS versioned service.
 *
 * @detail
 * Each FDS Thrift service can be independently versioned.
 * Provides a consistent interface for versioned services.
 *
 * See development rules and guidelines:
 *
 *   https://formationds.atlassian.net/wiki/display/ENG/Thrift+API+Versions
 */
service VersionedService {

  /**
   * @brief Gets the API version for the service handler
   */
  Version getVersion() throws (1: common.ApiException e);

  /**
   * @brief Gets the version table for the service.
   * @detail
   * The table will have a row for every major API version.
   *
   * @returns A list of service API versions or the empty list
   */
  list<ServiceAPIVersion> getVersionTable() throws (1: common.ApiException e);

  /**
   * @brief Given a suggested API version, returns an acceptable version.
   *        This enables the client and server to negotiate an API version
   *        in which to communicate.
   *
   * @param suggestedVersion: The service API version suggested by the client.
   *    Using Thrift, the client is the code using a Thrift generated service.client
   *    instance.
   *
   * @returns Version: The API version acceptable to the server
   */
  Version suggestVersion(1: required Version suggestedVersion) throws (1: common.ApiException e);
}
