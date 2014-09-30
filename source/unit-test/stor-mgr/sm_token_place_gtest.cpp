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
        ObjectLocationTable::ptr olt(new ObjectLocationTable());
        SmTokenPlacement::compute(hdds, ssds, olt);
        GLOGNORMAL << "Initial computation - " << *olt << std::endl;

        // test serialize
        std::string buffer;
        olt->getSerialized(buffer);

        ObjectLocationTable::ptr olt2(new ObjectLocationTable());
        olt2->loadSerialized(buffer);
        EXPECT_TRUE(*olt == *olt2);

        // if ssd count == 0, both olts must have invalid disk
        // ids on ssd row
        if (ssd_count == 0) {
            for (fds_token_id tok = 0; tok < SMTOKEN_COUNT; ++tok) {
                fds_uint16_t diskId = olt->getDiskId(tok, diskio::flashTier);
                EXPECT_FALSE(olt->isDiskIdValid(diskId));
                diskId = olt2->getDiskId(tok, diskio::flashTier);
                EXPECT_FALSE(olt->isDiskIdValid(diskId));
            }
        }

        // test comparison :)
        olt2->setDiskId(0, diskio::diskTier, 65000);
        EXPECT_FALSE(*olt == *olt2);
    }
}

}  // namespace fds

int main(int argc, char * argv[]) {
    fds::init_process_globals(fds::logname);
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

