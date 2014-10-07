/**
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <ostream>
#include <set>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <fds_process.h>
#include <object-store/SmSuperblock.h>

using ::testing::AtLeast;
using ::testing::Return;

namespace fds {


static const fds_uint32_t sbDefaultHddCount = 3;
static const fds_uint32_t sbDefaultSsdCount = 0;

static const std::string logname = "sm_superblock";

class SmSuperblockTestDriver {
  public:
    SmSuperblockTestDriver(fds_uint32_t hddCount,
                           fds_uint32_t ssdCount);
    SmSuperblockTestDriver()
            : SmSuperblockTestDriver(sbDefaultHddCount, sbDefaultSsdCount) {}
    ~SmSuperblockTestDriver();

    void createDirs();
    void deleteDirs();
    void loadSuperblock();
    void syncSuperblock();

  private:
    DiskLocMap diskMap;
    std::set<fds_uint16_t> hdds;
    std::set<fds_uint16_t> ssds;
    SmSuperblockMgr *sblock;
};  // class SmSuperblockTestDriver

SmSuperblockTestDriver::SmSuperblockTestDriver(fds_uint32_t hddCount,
                                               fds_uint32_t ssdCount)
{
    sblock = new SmSuperblockMgr();
    fds_uint16_t diskId = 1;

    /* Add disks from different tiers (ssd and hdd). */
    for (fds_uint32_t i = 0; i < hddCount; ++i) {
        diskMap[diskId] = "/tmp/hdd-" + std::to_string(i) + "/";
        hdds.insert(diskId);
        ++diskId;
    }
    for (fds_uint32_t i = 0; i < ssdCount; ++i) {
        diskMap[diskId] = "/tmp/ssd-" + std::to_string(i) + "/";
        ssds.insert(diskId);
        ++diskId;
    }
}

SmSuperblockTestDriver::~SmSuperblockTestDriver()
{
    delete sblock;
}

void
SmSuperblockTestDriver::createDirs()
{
    std::string dirPath;
    std::string filePath;

    GLOGNORMAL << "executing " << __func__ << std::endl;

    for (DiskLocMap::const_iterator cit = diskMap.cbegin();
         cit != diskMap.cend();
         ++cit) {
        /* Get both the directory (dev) path and file path
         */
        dirPath = cit->second;
        filePath = dirPath;
        filePath.append(sblock->SmSuperblockMgrTestGetFileName());

        /* create directories.
         */
        GLOGNORMAL << "Creating disk(" << cit->first << ") => " << dirPath << std::endl;
        if (mkdir(dirPath.c_str(), 0777) != 0) {
            /* If directory exists, then likely the superblock file exists.
             * Delete them to make it a pristine state.
             */
            if (errno == EEXIST) {
                GLOGNORMAL << "Directory "<< dirPath << "already exists" << std::endl;
                GLOGNORMAL << "Trying to removing file: " << filePath << std::endl;
                if (remove(filePath.c_str()) == 0) {
                    GLOGNORMAL << "Successfully removed file: " << filePath << std::endl;
                } else {
                    GLOGNORMAL << "Couldn't locate file: " << filePath << std::endl;
                }

            } else if (errno == EACCES) {
                GLOGNORMAL << "Access denied (" << dirPath << std::endl;
            }
        }
    }

    GLOGNORMAL << "completing " << __func__ << std::endl;
}

void
SmSuperblockTestDriver::deleteDirs()
{
    std::string dirPath;
    std::string filePath;

    GLOGNORMAL << "executing " << __func__ << std::endl;

    for (DiskLocMap::const_iterator cit = diskMap.cbegin();
         cit != diskMap.cend();
         ++cit) {
        dirPath = cit->second;
        filePath = dirPath;
        filePath.append(sblock->SmSuperblockMgrTestGetFileName());

        /* remove file and directory.  ignore return values.
         */
        if (remove(filePath.c_str()) == 0) {
            GLOGNORMAL << "Successfully removed file: " << filePath << std::endl;
        } else {
            GLOGNORMAL << "Couldn't remove file: " << filePath << std::endl;
        }
        if (remove(dirPath.c_str()) == 0) {
            GLOGNORMAL << "Successfully removed dir: " << dirPath << std::endl;
        } else {
            GLOGNORMAL << "Couldn't remove file: " << dirPath << std::endl;
        }
    }

    GLOGNORMAL << "completing " << __func__ << std::endl;

}

void
SmSuperblockTestDriver::loadSuperblock()
{
    SmTokenSet sm_toks;
    for (fds_token_id tok = 0; tok < SMTOKEN_COUNT; ++tok) {
        sm_toks.insert(tok);
    }

    GLOGNORMAL << "Loading Superblock" << std::endl;

    sblock->loadSuperblock(hdds, ssds, diskMap, sm_toks);
    GLOGNORMAL << *sblock << std::endl;
}

void
SmSuperblockTestDriver::syncSuperblock()
{
    std::cout << "Syncing Superblock" << std::endl;

    sblock->syncSuperblock();
}



TEST(SmSuperblockTestDriver, test1)
{
    GLOGNORMAL << "executing " << __func__ << std::endl;
    SmSuperblockTestDriver *test1 = new SmSuperblockTestDriver();

    test1->createDirs();
    test1->loadSuperblock();

    GLOGNORMAL << "completing " << __func__ << std::endl;
}

TEST(SmSuperblockTestDriver, test2)
{
    GLOGNORMAL << "executing " << __func__ << std::endl;
    SmSuperblockTestDriver *test1 = new SmSuperblockTestDriver();

    test1->loadSuperblock();

    GLOGNORMAL << "completing " << __func__ << std::endl;
}

TEST(SmSuperblockTestDriver, test3)
{
    GLOGNORMAL << "executing " << __func__ << std::endl;
    SmSuperblockTestDriver *test1 = new SmSuperblockTestDriver();

    test1->syncSuperblock();

    GLOGNORMAL << "completing " << __func__ << std::endl;
}






}  // fds

int
main(int argc, char *argv[])
{
    fds::init_process_globals(fds::logname);

    ::testing::InitGoogleMock(&argc, argv);

    return RUN_ALL_TESTS();
}


