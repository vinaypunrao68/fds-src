/**
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <unistd.h>
#include <set>
#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <fds_process.h>
#include <object-store/SmTokenPlacement.h>

using ::testing::AtLeast;
using ::testing::Return;

namespace fds {

static std::string logname = "smtokenplace";

void discoverDisks(fds_uint32_t hdd_count,
                   fds_uint32_t ssd_count,
                   std::set<fds_uint16_t>* ret_hdds,
                   std::set<fds_uint16_t>* ret_ssds) {
    std::set<fds_uint16_t> hdds;
    std::set<fds_uint16_t> ssds;
    fds_uint16_t disk_id = 1;
    for (fds_uint32_t i = 0; i < hdd_count; i++) {
        hdds.insert(disk_id);
        disk_id++;
    }
    for (fds_uint32_t i = 0; i < ssd_count; i++) {
        ssds.insert(disk_id);
        disk_id++;
    }

    ret_hdds->swap(hdds);
    ret_ssds->swap(ssds);
}

TEST(SmTokenPlacement, compute) {
    fds_uint32_t ssd_count = 2;
    fds_uint32_t hdd_count = 12;

    for (fds_uint32_t test_id = 0; test_id < 3; test_id++) {
        if (test_id == 0) {
            ssd_count = 0;
            hdd_count = 12;
        } else if (test_id == 1) {
            ssd_count = 2;
            hdd_count = 12;
        } else if (test_id == 2) {
            ssd_count = 0;
            hdd_count = 0;
        }
        GLOGNORMAL << hdd_count << " HDDs and "
                   << ssd_count << " SSDs";
        std::set<fds_uint16_t> hdds;
        std::set<fds_uint16_t> ssds;
        discoverDisks(hdd_count, ssd_count, &hdds, &ssds);

        // compute sm token placement
        ObjectLocationTable olt;
        SmTokenPlacement::compute(hdds, ssds, &olt);
        GLOGNORMAL << "Initial computation - " << olt << std::endl;

        // if ssd count == 0, olt must have invalid disk id on ssd row
        // otherwise disk id must be valid for each SM token
        for (fds_token_id tok = 0; tok < SMTOKEN_COUNT; ++tok) {
            fds_uint16_t diskId = olt.getDiskId(tok, diskio::flashTier);
            if (ssd_count == 0) {
                EXPECT_FALSE(olt.isDiskIdValid(diskId));
            } else {
                EXPECT_TRUE(olt.isDiskIdValid(diskId));
            }
        }

        // if we have hdds, every cell in hdd tier must be valid for every token
        if (hdd_count > 0) {
            for (fds_token_id tok = 0; tok < SMTOKEN_COUNT; ++tok) {
                fds_uint16_t diskId = olt.getDiskId(tok, diskio::diskTier);
                EXPECT_TRUE(olt.isDiskIdValid(diskId));
            }
        }
    }
}

TEST(SmTokenPlacement, empty) {
    GLOGNORMAL << "Testing SmTokenPlacement with no HDD and SSD";

    std::set<fds_uint16_t> hdds;
    std::set<fds_uint16_t> ssds;

    // newly created OLT contains invalid disk id in every cell
    ObjectLocationTable olt;
    for (fds_token_id tok = 0; tok < SMTOKEN_COUNT; ++tok) {
        fds_uint16_t diskId = olt.getDiskId(tok, diskio::diskTier);
        EXPECT_FALSE(olt.isDiskIdValid(diskId));
        diskId = olt.getDiskId(tok, diskio::flashTier);
        EXPECT_FALSE(olt.isDiskIdValid(diskId));
    }

    // compute when no disks
    SmTokenPlacement::compute(hdds, ssds, &olt);
    GLOGNORMAL << "Initial computation - " << olt << std::endl;

    // every cell should still contain invalid disk id
    for (fds_token_id tok = 0; tok < SMTOKEN_COUNT; ++tok) {
        fds_uint16_t diskId = olt.getDiskId(tok, diskio::diskTier);
        EXPECT_FALSE(olt.isDiskIdValid(diskId));
        diskId = olt.getDiskId(tok, diskio::flashTier);
        EXPECT_FALSE(olt.isDiskIdValid(diskId));
    }
}

TEST(SmTokenPlacement, comparison) {
    GLOGDEBUG << "Testing SmTokenPlacement comparison";

    std::set<fds_uint16_t> hdds;
    std::set<fds_uint16_t> ssds;
    discoverDisks(5, 2, &hdds, &ssds);

    // compute sm token placement
    ObjectLocationTable olt;
    SmTokenPlacement::compute(hdds, ssds, &olt);
    GLOGNORMAL << "Initial computation - " << olt << std::endl;

    // create token places for same set of disks
    ObjectLocationTable olt2;
    SmTokenPlacement::compute(hdds, ssds, &olt2);
    GLOGNORMAL << "Another OLT for same set of disks - " << olt2 << std::endl;

    // test comparison of same obj loc tables
    EXPECT_TRUE(olt == olt2);

    // change second OLT
    olt2.setDiskId(0, diskio::diskTier, 65000);
    EXPECT_FALSE(olt == olt2);
}

TEST(SmTokenPlacement, validation) {
    GLOGDEBUG << "Testing SmTokenPlacement validation";

    std::set<fds_uint16_t> hdds;
    std::set<fds_uint16_t> ssds;
    discoverDisks(6, 0, &hdds, &ssds);

    // compute sm token placement
    ObjectLocationTable olt;
    SmTokenPlacement::compute(hdds, ssds, &olt);
    GLOGNORMAL << "Initial computation - " << olt << std::endl;

    // validate for the same set of HDD and SSD devices
    Error err = olt.validate(hdds, diskio::diskTier);
    EXPECT_TRUE(err.ok());

    err = olt.validate(ssds, diskio::flashTier);
    EXPECT_TRUE(err.ok());

    // now discover new set of disks with one HDD gone
    // and one new SSD
    std::set<fds_uint16_t> hdds_new;
    std::set<fds_uint16_t> ssds_new;
    discoverDisks(5, 1, &hdds_new, &ssds_new);

    // we removed one disks -- expect error
    err = olt.validate(hdds_new, diskio::diskTier);
    EXPECT_TRUE(err == ERR_SM_OLT_DISKS_INCONSISTENT);

    // we added one ssd -- expect error
    err = olt.validate(ssds_new, diskio::flashTier);
    EXPECT_TRUE(err == ERR_SM_OLT_DISKS_INCONSISTENT);
}


}  // namespace fds

int main(int argc, char * argv[]) {
    fds::init_process_globals(fds::logname);
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

