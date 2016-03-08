/**
 * Copyright 2016 Formation Data Systems, Inc.
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "fdsp/fds_versioned_api_types.h"

// UUT
#include "fds_version.h"

TEST(ServiceAPIVersionTest, Assignment) {

    fds::ServiceAPIVersion::Aggregate a1{"domainService",
        "domainServiceBeta", "Beta", { 3, 2, 1 }
    };
    fds::ServiceAPIVersion v1(a1);
    EXPECT_EQ(v1.getServiceFamily(), "domainService");
    EXPECT_EQ(v1.getServiceName(), "domainServiceBeta");
    EXPECT_EQ(v1.getMajorAlias(), "Beta");
    EXPECT_EQ(v1.getVersion().getMajorVersion(), 3);
    EXPECT_EQ(v1.getVersion().getMinorVersion(), 2);
    EXPECT_EQ(v1.getVersion().getPatchVersion(), 1);

    // First assignment
    fds::ServiceAPIVersion v2 = v1;
    EXPECT_EQ(v2.getServiceFamily(), "domainService");
    EXPECT_EQ(v2.getServiceName(), "domainServiceBeta");
    EXPECT_EQ(v2.getMajorAlias(), "Beta");
    EXPECT_EQ(v2.getVersion().getMajorVersion(), 3);
    EXPECT_EQ(v2.getVersion().getMinorVersion(), 2);
    EXPECT_EQ(v2.getVersion().getPatchVersion(), 1);
}

TEST(ServiceAPIVersionTest, Copy) {

    fds::ServiceAPIVersion::Aggregate a1{"domainService",
        "domainServiceAlpha", "Alpha", { 3, 2, 1 }
    };
    fds::ServiceAPIVersion av1(a1);
    EXPECT_EQ(av1.getServiceFamily(), "domainService");
    EXPECT_EQ(av1.getServiceName(), "domainServiceAlpha");
    EXPECT_EQ(av1.getMajorAlias(), "Alpha");
    EXPECT_EQ(av1.getVersion().getMajorVersion(), 3);
    EXPECT_EQ(av1.getVersion().getMinorVersion(), 2);
    EXPECT_EQ(av1.getVersion().getPatchVersion(), 1);

    // Copy
    fds::ServiceAPIVersion av2(av1);
    EXPECT_EQ(av2.getServiceFamily(), "domainService");
    EXPECT_EQ(av2.getServiceName(), "domainServiceAlpha");
    EXPECT_EQ(av2.getMajorAlias(), "Alpha");
    EXPECT_EQ(av2.getVersion().getMajorVersion(), 3);
    EXPECT_EQ(av2.getVersion().getMinorVersion(), 2);
    EXPECT_EQ(av2.getVersion().getPatchVersion(), 1);
}

TEST(ServiceAPIVersionTest, Handshake) {

    std::vector<fds::Version::Aggregate> va = {
        { 0, 1, 0 }, // 0
        { 1, 0, 0 }, // 1
        { 1, 0, 1 }, // 2
        { 1, 1, 0 }, // 3
        { 1, 1, 1 }, // 4
        { 2, 0, 0 }, // 5
        { 2, 0, 1 }, // 6
        { 2, 1, 0 }, // 7
        { 2, 1, 1 }, // 8
        { 3, 0, 0 }  // 9
    };

    // Same version
    fds::Version v0 = fds::ServiceAPIVersion::handshake(fds::Version(va[0]),
        fds::Version(va[0]));
    EXPECT_EQ(v0.getMajorVersion(), 0);
    EXPECT_EQ(v0.getMinorVersion(), 1);
    EXPECT_EQ(v0.getPatchVersion(), 0);

    // New server, old client stubs
    fds::Version v1 = fds::ServiceAPIVersion::handshake(fds::Version(va[1]),
        fds::Version(va[0]));
    // Use suggested version
    EXPECT_EQ(v1.getMajorVersion(), 0);
    EXPECT_EQ(v1.getMinorVersion(), 1);
    EXPECT_EQ(v1.getPatchVersion(), 0);

    fds::Version v2 = fds::ServiceAPIVersion::handshake(fds::Version(va[3]),
        fds::Version(va[2]));
    // Use suggested version
    EXPECT_EQ(v2.getMajorVersion(), 1);
    EXPECT_EQ(v2.getMinorVersion(), 0);
    EXPECT_EQ(v2.getPatchVersion(), 1);

    fds::Version v3 = fds::ServiceAPIVersion::handshake(fds::Version(va[8]),
        fds::Version(va[7]));
    // Use suggested version
    EXPECT_EQ(v3.getMajorVersion(), 2);
    EXPECT_EQ(v3.getMinorVersion(), 1);
    EXPECT_EQ(v3.getPatchVersion(), 0);

    // Old server, new client stubs
    fds::Version v4 = fds::ServiceAPIVersion::handshake(fds::Version(va[2]),
        fds::Version(va[5]));
    // Use server version
    EXPECT_EQ(v4.getMajorVersion(), 1);
    EXPECT_EQ(v4.getMinorVersion(), 0);
    EXPECT_EQ(v4.getPatchVersion(), 1);
}

TEST(ServiceAPIVersion, Thrift) {

    fds::apis::ServiceAPIVersion tav1;
    tav1.service_family = "arbitraryService";
    tav1.service_name = "arbitraryServiceCodename";
    tav1.major_alias = "Codename";
    tav1.api_version.major_version = 6;
    tav1.api_version.minor_version = 7;
    tav1.api_version.patch_version = 8;

    // construct using thrift-generated object
    fds::ServiceAPIVersion av1(tav1);
    EXPECT_EQ(av1.getServiceFamily(), "arbitraryService");
    EXPECT_EQ(av1.getServiceName(), "arbitraryServiceCodename");
    EXPECT_EQ(av1.getMajorAlias(), "Codename");
    EXPECT_EQ(av1.getVersion().getMajorVersion(), 6);
    EXPECT_EQ(av1.getVersion().getMinorVersion(), 7);
    EXPECT_EQ(av1.getVersion().getPatchVersion(), 8);

    // produce thrift-generated object
    fds::apis::ServiceAPIVersion t = av1.toThrift();
    EXPECT_EQ(t.service_family, "arbitraryService");
    EXPECT_EQ(t.service_name, "arbitraryServiceCodename");
    EXPECT_EQ(t.major_alias, "Codename");
    EXPECT_EQ(t.api_version.major_version, 6);
    EXPECT_EQ(t.api_version.minor_version, 7);
    EXPECT_EQ(t.api_version.patch_version, 8);
}

TEST(VersionTest, Assignment) {

    fds::Version::Aggregate a1{ 100, 9, 99 };
    fds::Version v1;
    v1 = fds::Version(a1);
    EXPECT_EQ(v1.getMajorVersion(), 100);
    EXPECT_EQ(v1.getMinorVersion(), 9);
    EXPECT_EQ(v1.getPatchVersion(), 99);

    fds::Version::Aggregate a2{ 4, 3, 2 };
    fds::Version v2;
    // First assignment
    v2 = fds::Version(a2);
    EXPECT_EQ(v2.getMajorVersion(), 4);
    EXPECT_EQ(v2.getMinorVersion(), 3);
    EXPECT_EQ(v2.getPatchVersion(), 2);

    // Second assignment
    v2 = v1;
    EXPECT_EQ(v2.getMajorVersion(), 100);
    EXPECT_EQ(v2.getMinorVersion(), 9);
    EXPECT_EQ(v2.getPatchVersion(), 99);
}

TEST(VersionTest, AsString) {

    std::vector<fds::Version::Aggregate> v = {
        { 5, 6, 7 },        // 0
        { 7, 6, 5 },        // 1
        { 0, 0, 0 },        // 2
        { 11, 4, 4 },       // 3
        { 4, 11, 3 },       // 4
        { 5, 5, 14 },       // 5
        { 100, 101, 102 },  // 6
        { 100, 0, 0 },      // 7
        { 1000, 0, 999 },   // 8
        { -2, -3, -4 }      // 9 Nonsense, but check anyway
    };
    EXPECT_EQ(fds::Version(v[0]).toString(false), "5.6.7");
    EXPECT_EQ(fds::Version(v[0]).toString(true), "05.06.07");
    EXPECT_EQ(fds::Version(v[0]).toString(), "05.06.07");
    EXPECT_EQ(fds::Version(v[1]).toString(false), "7.6.5");
    EXPECT_EQ(fds::Version(v[1]).toString(true), "07.06.05");
    EXPECT_EQ(fds::Version(v[1]).toString(), "07.06.05");
    EXPECT_EQ(fds::Version(v[2]).toString(false), "0.0.0");
    EXPECT_EQ(fds::Version(v[2]).toString(true), "00.00.00");
    EXPECT_EQ(fds::Version(v[2]).toString(), "00.00.00");
    EXPECT_EQ(fds::Version(v[3]).toString(false), "11.4.4");
    EXPECT_EQ(fds::Version(v[3]).toString(true), "11.04.04");
    EXPECT_EQ(fds::Version(v[3]).toString(), "11.04.04");
    EXPECT_EQ(fds::Version(v[4]).toString(false), "4.11.3");
    EXPECT_EQ(fds::Version(v[4]).toString(true), "04.11.03");
    EXPECT_EQ(fds::Version(v[4]).toString(), "04.11.03");
    EXPECT_EQ(fds::Version(v[5]).toString(false), "5.5.14");
    EXPECT_EQ(fds::Version(v[5]).toString(true), "05.05.14");
    EXPECT_EQ(fds::Version(v[5]).toString(), "05.05.14");
    EXPECT_EQ(fds::Version(v[6]).toString(false), "100.101.102");
    EXPECT_EQ(fds::Version(v[6]).toString(true), "100.101.102");
    EXPECT_EQ(fds::Version(v[6]).toString(), "100.101.102");
    EXPECT_EQ(fds::Version(v[7]).toString(false), "100.0.0");
    EXPECT_EQ(fds::Version(v[7]).toString(true), "100.00.00");
    EXPECT_EQ(fds::Version(v[7]).toString(), "100.00.00");
    EXPECT_EQ(fds::Version(v[8]).toString(false), "1000.0.999");
    EXPECT_EQ(fds::Version(v[8]).toString(true), "1000.00.999");
    EXPECT_EQ(fds::Version(v[8]).toString(), "1000.00.999");
    EXPECT_EQ(fds::Version(v[9]).toString(false), "-2.-3.-4");
    EXPECT_EQ(fds::Version(v[9]).toString(true), "-2.-3.-4");
    EXPECT_EQ(fds::Version(v[9]).toString(), "-2.-3.-4");
}

TEST(VersionTest, Copy) {

    fds::Version::Aggregate a1{ 2, 3, 4 };
    fds::Version v1(a1);
    EXPECT_EQ(v1.getMajorVersion(), 2);
    EXPECT_EQ(v1.getMinorVersion(), 3);
    EXPECT_EQ(v1.getPatchVersion(), 4);

    // Copy constructor
    fds::Version v2(v1);
    EXPECT_EQ(v2.getMajorVersion(), 2);
    EXPECT_EQ(v2.getMinorVersion(), 3);
    EXPECT_EQ(v2.getPatchVersion(), 4);
}

TEST(VersionTest, Defaults) {

    // Default version tests
    fds::Version defaultVersion;
    EXPECT_EQ(defaultVersion.getMajorVersion(), 0);
    EXPECT_EQ(defaultVersion.getMinorVersion(), 1);
    EXPECT_EQ(defaultVersion.getPatchVersion(), 0);
    EXPECT_EQ(defaultVersion.toString(false), "0.1.0");
    EXPECT_EQ(defaultVersion.toString(true), "00.01.00");
    EXPECT_EQ(defaultVersion.toString(), "00.01.00");
    EXPECT_TRUE(defaultVersion == fds::Version());
    EXPECT_FALSE(defaultVersion != fds::Version());
}

TEST(VersionTest, Operators) {

    std::vector<fds::Version::Aggregate> v = {
        { 5, 6, 7 },  // 0
        { 7, 6, 5 },  // 1
        { 5, 6, 7 },  // 2
        { 1, 1, 1 },  // 3
        { 1, 1, 6 },  // 4
        { 1, 2, 1 },  // 5
        { 1, 3, 1 },  // 6
        { 11, 2, 2 }, // 7
        { 110, 2, 2}  // 8
    };
    EXPECT_EQ(fds::Version(v[0]).getMajorVersion(), 5);
    EXPECT_EQ(fds::Version(v[0]).getMinorVersion(), 6);
    EXPECT_EQ(fds::Version(v[0]).getPatchVersion(), 7);
    EXPECT_EQ(fds::Version(v[1]).getMajorVersion(), 7);
    EXPECT_EQ(fds::Version(v[1]).getMinorVersion(), 6);
    EXPECT_EQ(fds::Version(v[1]).getPatchVersion(), 5);
    EXPECT_TRUE (fds::Version(v[0]) == fds::Version(v[0]));
    EXPECT_FALSE(fds::Version(v[0]) != fds::Version(v[0]));
    EXPECT_FALSE(fds::Version(v[0]) == fds::Version(v[1]));
    EXPECT_TRUE (fds::Version(v[0]) != fds::Version(v[1]));
    EXPECT_TRUE (fds::Version(v[0]) == fds::Version(v[2]));
    EXPECT_FALSE(fds::Version(v[0]) != fds::Version(v[2]));
    EXPECT_FALSE(fds::Version(v[3]) == fds::Version(v[4]));
    EXPECT_TRUE (fds::Version(v[3]) != fds::Version(v[4]));
    EXPECT_FALSE(fds::Version(v[5]) == fds::Version(v[6]));
    EXPECT_TRUE (fds::Version(v[5]) != fds::Version(v[6]));
    EXPECT_FALSE(fds::Version(v[7]) == fds::Version(v[8]));
    EXPECT_TRUE (fds::Version(v[7]) != fds::Version(v[8]));
}

TEST(VersionTest, Thrift) {

    fds::apis::Version tv1;
    tv1.major_version = 6;
    tv1.minor_version = 7;
    tv1.patch_version = 8;
    // construct using thrift-generated object
    fds::Version v1(tv1);
    EXPECT_EQ(v1.getMajorVersion(), 6);
    EXPECT_EQ(v1.getMinorVersion(), 7);
    EXPECT_EQ(v1.getPatchVersion(), 8);

    // produce thrift-generated object
    fds::apis::Version troundtrip = v1.toThrift();
    EXPECT_EQ(troundtrip.major_version, 6);
    EXPECT_EQ(troundtrip.minor_version, 7);
    EXPECT_EQ(troundtrip.patch_version, 8);
}

int main(int argc, char* argv[]) {

    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

