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
#include <persistent-layer/persistentdata.h>

using ::testing::AtLeast;
using ::testing::Return;

namespace fds {

static std::string logname = "sm_token_place";

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

    for (fds_uint32_t test_id = 0; test_id < 7; test_id++) {
        if (test_id == 0) {
            ssd_count = 0;
            hdd_count = 12;
        } else if (test_id == 1) {
            ssd_count = 2;
            hdd_count = 12;
        } else if (test_id == 2) {
            ssd_count = 0;
            hdd_count = 0;
        } else if ((test_id >= 3) && (test_id <= 5)) {
            ssd_count = test_id - 2 + (test_id - 3)*6;
            hdd_count = 0;
        } else if (test_id == 6) {
            ssd_count = 1;
            hdd_count = 14;
        }
        GLOGNORMAL << hdd_count << " HDDs and "
                   << ssd_count << " SSDs";
        std::set<fds_uint16_t> hdds;
        std::set<fds_uint16_t> ssds;
        discoverDisks(hdd_count, ssd_count, &hdds, &ssds);

        // compute sm token placement
        ObjectLocationTable olt;
        SmTokenPlacement::compute(hdds, ssds, &olt);
        GLOGNORMAL << "Initial computation - " << olt;

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

        // if hdd count == 0, olt must have invalid disk id on HDD row
        // otherwise disk id must be valid for each SM token
        for (fds_token_id tok = 0; tok < SMTOKEN_COUNT; ++tok) {
            fds_uint16_t diskId = olt.getDiskId(tok, diskio::diskTier);
            if (hdd_count == 0) {
                EXPECT_FALSE(olt.isDiskIdValid(diskId));
            } else {
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
    GLOGNORMAL << "Initial computation - " << olt;

    // every cell should still contain invalid disk id
    for (fds_token_id tok = 0; tok < SMTOKEN_COUNT; ++tok) {
        fds_uint16_t diskId = olt.getDiskId(tok, diskio::diskTier);
        EXPECT_FALSE(olt.isDiskIdValid(diskId));
        diskId = olt.getDiskId(tok, diskio::flashTier);
        EXPECT_FALSE(olt.isDiskIdValid(diskId));
    }
}

TEST(SmTokenPlacement, comparison) {
    GLOGNORMAL << "Testing SmTokenPlacement comparison";

    std::set<fds_uint16_t> hdds;
    std::set<fds_uint16_t> ssds;
    discoverDisks(5, 2, &hdds, &ssds);

    // compute sm token placement
    ObjectLocationTable olt;
    SmTokenPlacement::compute(hdds, ssds, &olt);
    GLOGNORMAL << "Initial computation - " << olt;

    // create token places for same set of disks
    ObjectLocationTable olt2;
    SmTokenPlacement::compute(hdds, ssds, &olt2);
    GLOGNORMAL << "Another OLT for same set of disks - " << olt2;

    // test comparison of same obj loc tables
    EXPECT_TRUE(olt == olt2);

    // change second OLT
    olt2.setDiskId(0, diskio::diskTier, 65000);
    EXPECT_FALSE(olt == olt2);
}

TEST(SmTokenPlacement, validation) {
    GLOGNORMAL << "Testing SmTokenPlacement validation";

    std::set<fds_uint16_t> hdds;
    std::set<fds_uint16_t> ssds;
    discoverDisks(6, 0, &hdds, &ssds);

    // compute sm token placement
    ObjectLocationTable olt;
    SmTokenPlacement::compute(hdds, ssds, &olt);
    GLOGNORMAL << "Initial computation - " << olt;

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

// 1) 1, 2 => remove 1
TEST(SmTokenPlacement, recompute1)
{
    GLOGNORMAL << "Testing SmTokenPlacement recompute";

    Error err(ERR_OK);
    std::set<fds_uint16_t> baseHdds;
    std::set<fds_uint16_t> baseSsds;
    discoverDisks(2, 0, &baseHdds, &baseSsds);

    // compute sm token placement
    ObjectLocationTable olt;
    SmTokenPlacement::compute(baseHdds, baseSsds, &olt);
    //GLOGNORMAL << olt;

    SmTokenMultiSet totalTokens;
    for (auto diskId : baseHdds) {
        SmTokenSet tmpTokens = olt.getSmTokens(diskId);
        if (tmpTokens.size() > 0) {
            totalTokens.insert(tmpTokens.begin(), tmpTokens.end());
        }
    }

    EXPECT_EQ(SMTOKEN_COUNT, totalTokens.size());

    std::set<fds_uint16_t> addedHdds;
    std::set<fds_uint16_t> removedHdds;
    removedHdds.insert(1);

    SmTokenSet toks = olt.getSmTokens(1);
    GLOGNORMAL << "tokens on disk 1 == " << toks.size() << std::endl;
    DiskLocMap diskLocMap;
    SmTokenPlacement::recompute(baseHdds,
                                addedHdds,
                                removedHdds,
                                diskio::diskTier,
                                &olt,
                                diskLocMap,
                                err);

    //GLOGNORMAL << olt;

    SmTokenSet finalTokens = olt.getSmTokens(2);
    EXPECT_EQ(SMTOKEN_COUNT, finalTokens.size());

    std::set<uint16_t> diskSet = olt.getDiskSet(diskio::diskTier);
    std::set<uint16_t>::iterator diskSetIter = diskSet.find(1);
    ASSERT_TRUE(diskSetIter == diskSet.end());
    diskSetIter = diskSet.find(2);
    ASSERT_FALSE(diskSetIter == diskSet.end());
}

// 2) 1, 2 => add 3
TEST(SmTokenPlacement, recompute2)
{
    GLOGNORMAL << "Testing SmTokenPlacement recompute2";
    Error err(ERR_OK);

    std::set<fds_uint16_t> baseHdds;
    std::set<fds_uint16_t> baseSsds;
    discoverDisks(2, 0, &baseHdds, &baseSsds);

    // compute sm token placement
    ObjectLocationTable olt;
    SmTokenPlacement::compute(baseHdds, baseSsds, &olt);
    //GLOGNORMAL << olt;

    SmTokenMultiSet preTotalTokens;
    for (auto diskId : baseHdds) {
        SmTokenSet tmpTokens = olt.getSmTokens(diskId);
        if (tmpTokens.size() > 0) {
            preTotalTokens.insert(tmpTokens.begin(), tmpTokens.end());
        }
    }

    EXPECT_EQ(SMTOKEN_COUNT, preTotalTokens.size());

    std::set<fds_uint16_t> addedHdds;
    std::set<fds_uint16_t> removedHdds;
    addedHdds.insert(3);

    DiskLocMap diskLocMap;
    SmTokenPlacement::recompute(baseHdds,
                                addedHdds,
                                removedHdds,
                                diskio::diskTier,
                                &olt,
                                diskLocMap, 
                                err);

    //GLOGNORMAL << olt;

    std::set<fds_uint16_t> totalHdds;
    totalHdds.insert(baseHdds.begin(), baseHdds.end());
    totalHdds.insert(3);

    SmTokenMultiSet postTotalTokens;
    for (auto diskId : totalHdds) {
        SmTokenSet tmpTokens = olt.getSmTokens(diskId);
        if (tmpTokens.size() > 0) {
            postTotalTokens.insert(tmpTokens.begin(), tmpTokens.end());
        }
    }
    auto disk3Tokens = olt.getNumSmTokens(3);
    disk3Tokens += SMTOKEN_COUNT;
    EXPECT_EQ(disk3Tokens, postTotalTokens.size());

    std::set<uint16_t> diskSet = olt.getDiskSet(diskio::diskTier);
    std::set<uint16_t>::iterator diskSetIter = diskSet.find(3);
    ASSERT_FALSE(diskSetIter == diskSet.end());
}

// 3) 1, 2, 3 => remove 1, add 4
TEST(SmTokenPlacement, recompute3)
{
    GLOGNORMAL << "Testing SmTokenPlacement recompute3";

    Error err(ERR_OK);
    std::set<fds_uint16_t> baseHdds;
    std::set<fds_uint16_t> baseSsds;
    discoverDisks(3, 0, &baseHdds, &baseSsds);

    // compute sm token placement
    ObjectLocationTable olt;
    SmTokenPlacement::compute(baseHdds, baseSsds, &olt);
    //GLOGNORMAL << olt;

    SmTokenMultiSet preTotalTokens;
    for (auto diskId : baseHdds) {
        SmTokenSet tmpTokens = olt.getSmTokens(diskId);
        if (tmpTokens.size() > 0) {
            preTotalTokens.insert(tmpTokens.begin(), tmpTokens.end());
        }
    }
    EXPECT_EQ(SMTOKEN_COUNT, preTotalTokens.size());

    std::set<fds_uint16_t> addedHdds;
    std::set<fds_uint16_t> removedHdds;
    removedHdds.insert(1);
    addedHdds.insert(4);

    DiskLocMap diskLocMap;
    SmTokenPlacement::recompute(baseHdds,
                                addedHdds,
                                removedHdds,
                                diskio::diskTier,
                                &olt,
                                diskLocMap, 
                                err);

    //GLOGNORMAL << olt;

    std::set<fds_uint16_t> totalHdds;
    totalHdds.insert(baseHdds.begin(), baseHdds.end());
    totalHdds.insert(4);
    totalHdds.erase(1);

    SmTokenMultiSet postTotalTokens;
    for (auto diskId : totalHdds) {
        SmTokenSet tmpTokens = olt.getSmTokens(diskId);
        if (tmpTokens.size() > 0) {
            postTotalTokens.insert(tmpTokens.begin(), tmpTokens.end());
        }
    }

    EXPECT_EQ(SMTOKEN_COUNT, postTotalTokens.size());

    std::set<uint16_t> diskSet = olt.getDiskSet(diskio::diskTier);
    std::set<uint16_t>::iterator diskSetIter;
    diskSetIter = diskSet.find(1);
    ASSERT_TRUE(diskSetIter == diskSet.end());
    diskSetIter = diskSet.find(4);
    ASSERT_FALSE(diskSetIter == diskSet.end());

}


// 4) 1, 2, 3, 4 -> remove 1, 2 add 5, 6, 7
TEST(SmTokenPlacement, recompute4)
{
    GLOGNORMAL << "Testing SmTokenPlacement recompute4";

    Error err(ERR_OK);
    std::set<fds_uint16_t> baseHdds;
    std::set<fds_uint16_t> baseSsds;
    discoverDisks(4, 0, &baseHdds, &baseSsds);

    // compute sm token placement
    ObjectLocationTable olt;
    SmTokenPlacement::compute(baseHdds, baseSsds, &olt);
    //GLOGNORMAL << olt;

    SmTokenMultiSet preTotalTokens;
    for (auto diskId : baseHdds) {
        SmTokenSet tmpTokens = olt.getSmTokens(diskId);
        if (tmpTokens.size() > 0) {
            preTotalTokens.insert(tmpTokens.begin(), tmpTokens.end());
        }
        olt.printSmTokens(diskId);
    }
    EXPECT_EQ(SMTOKEN_COUNT, preTotalTokens.size());

    std::set<fds_uint16_t> addedHdds;
    std::set<fds_uint16_t> removedHdds;
    removedHdds.insert(1);
    removedHdds.insert(2);
    addedHdds.insert(5);
    addedHdds.insert(6);
    addedHdds.insert(7);

    auto disk1Tokens = olt.getNumSmTokens(1);
    auto disk2Tokens = olt.getNumSmTokens(2);
    DiskLocMap diskLocMap;
    SmTokenPlacement::recompute(baseHdds,
                                addedHdds,
                                removedHdds,
                                diskio::diskTier,
                                &olt,
                                diskLocMap,
                                err);

    //GLOGNORMAL << olt;

    std::set<fds_uint16_t> totalHdds;
    totalHdds.insert(baseHdds.begin(), baseHdds.end());
    totalHdds.erase(1);
    totalHdds.erase(2);
    totalHdds.insert(5);
    totalHdds.insert(6);
    totalHdds.insert(7);

    SmTokenMultiSet postTotalTokens;
    for (auto diskId : totalHdds) {
        SmTokenSet tmpTokens = olt.getSmTokens(diskId);
        if (tmpTokens.size() > 0) {
            postTotalTokens.insert(tmpTokens.begin(), tmpTokens.end());
        }
        olt.printSmTokens(diskId);
    }

    auto disk5Tokens = olt.getNumSmTokens(5);
    auto disk6Tokens = olt.getNumSmTokens(6);
    auto disk7Tokens = olt.getNumSmTokens(7);
    auto totalTokens = (SMTOKEN_COUNT + disk5Tokens +
                        disk6Tokens + disk7Tokens) -
                        (disk1Tokens + disk2Tokens);
    EXPECT_EQ(totalTokens, postTotalTokens.size());

    std::set<uint16_t> diskSet = olt.getDiskSet(diskio::diskTier);
    std::set<uint16_t>::iterator diskSetIter;
    diskSetIter = diskSet.find(1);
    ASSERT_TRUE(diskSetIter == diskSet.end());
    diskSetIter = diskSet.find(2);
    ASSERT_TRUE(diskSetIter == diskSet.end());
    diskSetIter = diskSet.find(5);
    ASSERT_FALSE(diskSetIter == diskSet.end());
    diskSetIter = diskSet.find(6);
    ASSERT_FALSE(diskSetIter == diskSet.end());
    diskSetIter = diskSet.find(7);
    ASSERT_FALSE(diskSetIter == diskSet.end());
}


// 5) 1, 2, 3, 4 -> remove 3
TEST(SmTokenPlacement, recompute5)
{
    GLOGNORMAL << "Testing SmTokenPlacement recompute5";

    Error err(ERR_OK);
    std::set<fds_uint16_t> baseHdds;
    std::set<fds_uint16_t> baseSsds;
    discoverDisks(4, 0, &baseHdds, &baseSsds);

    // compute sm token placement
    ObjectLocationTable olt;
    SmTokenPlacement::compute(baseHdds, baseSsds, &olt);
    //GLOGNORMAL << olt;

    SmTokenMultiSet preTotalTokens;
    for (auto diskId : baseHdds) {
        SmTokenSet tmpTokens = olt.getSmTokens(diskId);
        if (tmpTokens.size() > 0) {
            preTotalTokens.insert(tmpTokens.begin(), tmpTokens.end());
        }
    }
    EXPECT_EQ(SMTOKEN_COUNT, preTotalTokens.size());

    std::set<fds_uint16_t> addedHdds;
    std::set<fds_uint16_t> removedHdds;
    removedHdds.insert(3);

    DiskLocMap diskLocMap;
    SmTokenPlacement::recompute(baseHdds,
                                addedHdds,
                                removedHdds,
                                diskio::diskTier,
                                &olt,
                                diskLocMap,
                                err);

    //GLOGNORMAL << olt;

    std::set<fds_uint16_t> totalHdds;
    totalHdds.insert(baseHdds.begin(), baseHdds.end());
    totalHdds.erase(3);

    SmTokenMultiSet postTotalTokens;
    for (auto diskId : totalHdds) {
        SmTokenSet tmpTokens = olt.getSmTokens(diskId);
        if (tmpTokens.size() > 0) {
            postTotalTokens.insert(tmpTokens.begin(), tmpTokens.end());
        }
    }

    EXPECT_EQ(SMTOKEN_COUNT, postTotalTokens.size());

    std::set<uint16_t> diskSet = olt.getDiskSet(diskio::diskTier);
    std::set<uint16_t>::iterator diskSetIter;
    diskSetIter = diskSet.find(3);
    ASSERT_TRUE(diskSetIter == diskSet.end());
}


}  // namespace fds

int main(int argc, char * argv[]) {
    fds::init_process_globals(fds::logname);
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}

