/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_FDS_VERSION_H_
#define SOURCE_INCLUDE_FDS_VERSION_H_

#include <string>

#include <fdsp/fds_versioned_api_types.h>

namespace fds {

/**
 * Follows Semantic Versioning Specification (SemVer) http://semver.org
 */
class Version final {

private:
    int32_t majorVersion_;
    int32_t minorVersion_;
    int32_t patchVersion_;

public:

    /**
     * The Thrift-generated fds::apis::Version has a virtual destructor.
     * It therefore does not support brace initialization. Here is a
     * convenience structure that does support brace initialization.
     */
    struct Aggregate {
        int32_t majorVersion_;
        int32_t minorVersion_;
        int32_t patchVersion_;
    };

    /**
     * @brief Produces a default version '0.1.0'.
     */
    Version();
    Version(const Version& other);
    Version& operator=(const Version& other);

    /**
     * @brief A helper that enables brace initialization
     */
    explicit Version(const Aggregate& ag);

    /**
     * @brief Creates version from Thrift-generated
     */
    explicit Version(const fds::apis::Version& tVersion);

    ~Version() {}

    /**
     * @brief Produces object for use with Thrift API
     */
    fds::apis::Version toThrift() const;

    /**
     * @brief Useful for logging API versions, among other uses
     * @detail
     * By default, produces a string with leading zeros of the form '00.01.00'.
     * Optionally produces SemVer specified '0.1.0'.
     */
    std::string toString(bool showLeadingZeros = true) const;

    int32_t getMajorVersion() const { return majorVersion_; }
    int32_t getMinorVersion() const { return minorVersion_; }
    int32_t getPatchVersion() const { return patchVersion_; }

    bool operator==(const Version& rhs) const;
    bool operator!=(const Version& rhs) const;

    friend std::ostream& operator<<(std::ostream& out, const Version& version);
};

/**
 * @brief A record in a service API version table.
 * @detail
 * Helps Thrift client stub negotiate version with server.
 * Developer rules and guidelines for service API versions are documented in
 * confluence:
 *
 *  https://formationds.atlassian.net/wiki/display/ENG/Thrift+API+Versions
 */
class ServiceAPIVersion final {

private:
    /**
     * @brief Invariant for all versions of the service.
     */
    std::string serviceFamily_;

    /**
     * @brief The actual service name used to multiplex.
     */
    std::string serviceName_;

    /**
     * @brief Codename for the major version.
     */ 
    std::string majorAlias_;

    /**
     * @brief The SemVer data for the API.
     */
    Version apiVersion_;

    ServiceAPIVersion() = delete;

public:

    /**
     * The Thrift-generated fds::apis::ServiceAPIVersion has a virtual
     * destructor. It therefore does not support brace initialization.
     * Here is a convenience structure that does support brace 
     * initialization.
     */
    struct Aggregate {

        std::string serviceFamily_;
        std::string serviceName_;
        std::string majorAlias_;

        Version::Aggregate  apiVersion_;
    };

    ServiceAPIVersion(const ServiceAPIVersion& other);
    ServiceAPIVersion& operator=(const ServiceAPIVersion& other);

    /**
     * @brief A helper that enables brace initialization
     */
    explicit ServiceAPIVersion(const Aggregate& ag);

    /**
     * @brief Creates service API version from Thrift-generated
     */
    explicit ServiceAPIVersion(const fds::apis::ServiceAPIVersion& tServiceAPIVersion);

    ~ServiceAPIVersion() {} 

    /**
     * @brief Produces object for use with Thrift API
     */
    fds::apis::ServiceAPIVersion toThrift() const;

    const Version& getVersion() const { return apiVersion_; }

    std::string getServiceFamily() const { return serviceFamily_; }
    std::string getServiceName() const { return serviceName_; }
    std::string getMajorAlias() const { return majorAlias_; }

    /**
     * @brief Negotiates API version given client suggested version.
     * @detail
     * Using Thrift, the client is the code that uses a Thrift generated
     * service.client instance.
     * @param versionHere The API version for the service handler
     * @param versionSuggested The API version suggested by the client
     * @return Version An API version acceptable to the server
     */
    static Version handshake(const Version& versionHere, const Version& versionSuggested);
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_FDS_VERSION_H_
