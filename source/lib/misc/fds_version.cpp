/*
 * Copyright 2016 Formation Data Systems, Inc.
 */
#include "fds_version.h"

namespace fds {

Version::Version() : majorVersion_(0), minorVersion_(1), patchVersion_(0) {
}

Version::Version(const Version& other) : 
    majorVersion_(other.majorVersion_),
    minorVersion_(other.minorVersion_), 
    patchVersion_(other.patchVersion_) {
}

Version& Version::operator=(const Version& other) {
    if (&other != this) {
        majorVersion_ = other.majorVersion_;
        minorVersion_ = other.minorVersion_;
        patchVersion_ = other.patchVersion_;
    }
    return *this;
}

Version::Version(const Version::Aggregate& ag) :
    majorVersion_(ag.majorVersion_),
    minorVersion_(ag.minorVersion_), 
    patchVersion_(ag.patchVersion_) {
}

Version::Version(const fds::apis::Version& tVersion) :
    majorVersion_(tVersion.major_version),
    minorVersion_(tVersion.minor_version),
    patchVersion_(tVersion.patch_version) {
}

fds::apis::Version Version::toThrift() const {
    fds::apis::Version result;
    result.major_version = getMajorVersion();
    result.minor_version = getMinorVersion();
    result.patch_version = getPatchVersion();
    return result;
}

bool Version::operator==(const Version& rhs) const {
    if (&rhs == this) {
        return true;
    }
    if (getMajorVersion() == rhs.getMajorVersion()) {
        if (getMinorVersion() == rhs.getMinorVersion()) {
            if (getPatchVersion() == rhs.getPatchVersion()) {
                return true;
            }
        }
    }
    return false;
}

bool Version::operator!=(const Version& rhs) const {
    if (&rhs == this) {
        return false;
    }
    if (getMajorVersion() == rhs.getMajorVersion()) {
        if (getMinorVersion() == rhs.getMinorVersion()) {
            if (getPatchVersion() == rhs.getPatchVersion()) {
                return false;
            }
        }
    }
    return true;
}

// ============================================================================
//  \/  \/  \/  \/  \/  \/  \/  \/  \/  \/  \/  \/  \/  \/  \/  \/  \/  \/  \/
// ============================================================================

ServiceAPIVersion::ServiceAPIVersion(const ServiceAPIVersion& other) :
    serviceFamily_(other.serviceFamily_),
    serviceName_(other.serviceName_),
    majorAlias_(other.majorAlias_),
    apiVersion_(other.apiVersion_) {
}

ServiceAPIVersion& ServiceAPIVersion::operator=(const ServiceAPIVersion& other) {
    if (&other != this) {
        serviceFamily_ = other.serviceFamily_;
        serviceName_ = other.serviceName_;
        majorAlias_ = other.majorAlias_;
        apiVersion_ = other.apiVersion_;
    }
    return *this;
}

ServiceAPIVersion::ServiceAPIVersion(const ServiceAPIVersion::Aggregate& ag) :
    serviceFamily_(ag.serviceFamily_),
    serviceName_(ag.serviceName_),
    majorAlias_(ag.majorAlias_),
    apiVersion_(Version(ag.apiVersion_)) {
}

ServiceAPIVersion::ServiceAPIVersion(const fds::apis::ServiceAPIVersion& tServiceAPIVersion) :
    serviceFamily_(tServiceAPIVersion.service_family),
    serviceName_(tServiceAPIVersion.service_name),
    majorAlias_(tServiceAPIVersion.major_alias),
    apiVersion_(Version(tServiceAPIVersion.api_version)) {

}

fds::apis::ServiceAPIVersion ServiceAPIVersion::toThrift() const {
    fds::apis::ServiceAPIVersion result;
    result.service_family = getServiceFamily();
    result.service_name = getServiceName();
    result.major_alias = getMajorAlias();
    result.api_version.major_version = getVersion().getMajorVersion();
    result.api_version.minor_version = getVersion().getMinorVersion();
    result.api_version.patch_version = getVersion().getPatchVersion();
    return result;
}

}  // namespace fds

