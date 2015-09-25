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
#include <net/SvcProcess.h>

using ::testing::AtLeast;
using ::testing::Return;

namespace fds {


static const fds_uint32_t sbDefaultHddCount = 3;
static const fds_uint32_t sbDefaultSsdCount = 0;

class SmSuperblockUtProc : public SvcProcess {
  public:
    SmSuperblockUtProc(int argc, char * argv[], const std::string & config,
                    const std::string & basePath, Module * vec[]) {
        init(argc, argv, config, basePath, "sm_superblock.log", vec);
    }

    virtual int run() override {
        return 0;
    }
};

static const std::string logname = "sm_superblock";

class SmSuperblockTestDriver {
  public:
    SmSuperblockTestDriver(fds_uint32_t hddCount,
                           fds_uint32_t ssdCount);
    SmSuperblockTestDriver()
            : SmSuperblockTestDriver(sbDefaultHddCount, sbDefaultSsdCount) {}
    ~SmSuperblockTestDriver();

    void createDirs() {
        createDirs(false);
    }
    void createDirs(bool preserveDirs);
    void deleteDirs();
    void loadSuperblock();
    void syncSuperblock();

    void corruptSuperblockChecksum(std::string &path);
    void corruptSuperblockHeader(std::string& path);
    void corruptSuperblockOLT(std::string& path);
    void corruptSuperblockChangeVersion(std::string& path);
    void corruptSuperblockAnywhere(std::string& path);

    void removeSuperblock(std::string& path);
    bool compareFiles(const std::string& filePath1,
                      const std::string& filePath2);
    std::string getDiskPath(uint16_t diskOffset);

    SmTokenSet getSmTokens(fds_uint16_t diskId);
    bool verifyOLT();

    Error setDLTVersion(fds_uint64_t dltVersion, bool syncNow);
    fds_uint64_t getDLTVersion();

    std::string getSmSuperblockFileName() {
        return (sblock->SmSuperblockMgrTestGetFileName());
    };

  private:  // methods
    void readSuperblockToBuffer(std::string& path);
    void writeSuperblockFromBuffer(std::string& path);
    void corruptByte(off_t begOffset, off_t lenOffset);

  private:
    DiskLocMap diskMap;
    std::set<fds_uint16_t> hdds;
    std::set<fds_uint16_t> ssds;
    SmSuperblockMgr *sblock;
    char *fileBuffer;
    size_t fileBufferSize;
};  // class SmSuperblockTestDriver

SmSuperblockTestDriver::SmSuperblockTestDriver(fds_uint32_t hddCount,
                                               fds_uint32_t ssdCount)
{
    sblock = new SmSuperblockMgr();
    fds_uint16_t diskId = 1;
    fileBuffer = NULL;
    fileBufferSize = 0;

    /* Add disks from different tiers (ssd and hdd). */
    for (fds_uint32_t i = 0; i < hddCount; ++i) {
        diskMap[diskId] = "/tmp/hdd-" + std::to_string(i);
        hdds.insert(diskId);
        ++diskId;
    }
    for (fds_uint32_t i = 0; i < ssdCount; ++i) {
        diskMap[diskId] = "/tmp/ssd-" + std::to_string(i);
        ssds.insert(diskId);
        ++diskId;
    }
}

SmSuperblockTestDriver::~SmSuperblockTestDriver()
{
    delete sblock;
    delete fileBuffer;
}

void
SmSuperblockTestDriver::createDirs(bool preserveDirs)
{
    std::string dirPath;
    std::string filePath;

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
        GLOGNORMAL << "Creating disk(" << cit->first << ") => " << dirPath;
        if (mkdir(dirPath.c_str(), 0777) != 0) {
            /* If directory exists, then likely the superblock file exists.
             * Delete them to make it a pristine state.
             */
            if (errno == EEXIST) {
                GLOGNORMAL << "Directory "<< dirPath << "already exists";
                GLOGNORMAL << "Trying to removing file: " << filePath;
                if (!preserveDirs) {
                    if (remove(filePath.c_str()) == 0) {
                        GLOGNORMAL << "Successfully removed file: " << filePath;
                    } else {
                        GLOGNORMAL << "Couldn't locate file: " << filePath;
                    }
                }

            } else if (errno == EACCES) {
                GLOGNORMAL << "Access denied (" << dirPath;
            }
        }
    }
}

void
SmSuperblockTestDriver::deleteDirs()
{
    std::string dirPath;
    std::string filePath;

    for (DiskLocMap::const_iterator cit = diskMap.cbegin();
         cit != diskMap.cend();
         ++cit) {
        dirPath = cit->second;
        filePath = dirPath + "/" + sblock->SmSuperblockMgrTestGetFileName();

        /* remove file and directory.  ignore return values.
         */
        if (remove(filePath.c_str()) == 0) {
            GLOGNORMAL << "Successfully removed file: " << filePath;
        } else {
            GLOGNORMAL << "Couldn't remove file: " << filePath;
        }
        if (remove(dirPath.c_str()) == 0) {
            GLOGNORMAL << "Successfully removed dir: " << dirPath;
        } else {
            GLOGNORMAL << "Couldn't remove file: " << dirPath;
        }
    }
}

std::string
SmSuperblockTestDriver::getDiskPath(uint16_t diskOffset = 1)
{
    std::string diskPath;

    try {
        diskPath = diskMap[diskOffset];
    }
    catch (const std::out_of_range& oor) {
        fds_panic("diskId(%u) out of range\n", diskOffset);
    }

    return diskPath;
}


void
SmSuperblockTestDriver::readSuperblockToBuffer(std::string& path)
{
    std::ifstream fileStr(path.c_str());

    if (fileStr.good()) {
        fileStr.seekg(0, fileStr.end);
        fileBufferSize = fileStr.tellg();
        fds_verify(fileBufferSize == sizeof(struct SmSuperblock));
        fileStr.seekg(0, fileStr.beg);
        fileBuffer = new char [fileBufferSize];
        fileStr.read(fileBuffer, sizeof(struct SmSuperblock));
    } else {
        fds_panic("Cannot read a file %s\n", path.c_str());
    }

}

void
SmSuperblockTestDriver::writeSuperblockFromBuffer(std::string& path)
{
    fds_verify(NULL != fileBuffer);

    std::ofstream fileStr(path.c_str());

    if (fileStr.good()) {
        fileStr.write(fileBuffer, fileBufferSize);
        fds_verify(fileStr.tellp() == sizeof(struct SmSuperblock));
        fileStr.flush();
    } else {
        fds_panic("Cannot write a file %s\n", path.c_str());
    }

    delete fileBuffer;
    fileBuffer = NULL;
    fileBufferSize = 0;

}
void
SmSuperblockTestDriver::corruptByte(off_t begOffset,
                                    off_t lenOffset)
{
    /* going to corrupt a byte in random offset.
     */
    off_t corrupt_offset = begOffset + (rand() % lenOffset);

    /* Just complement the bits in the offset
     */
    fileBuffer[corrupt_offset] = ~fileBuffer[corrupt_offset] & 0xff;
}


void
SmSuperblockTestDriver::corruptSuperblockChecksum(std::string &path)
{
    fds_verify(NULL == fileBuffer);
    fds_checksum32_t *chksum_ptr;

    /* First 4 bytes of SM superblock contains the checksum.
     * Just fill it with random 32bit number.
     */
    readSuperblockToBuffer(path);

    /* Assuming that the first 32bits are for superblock checksum.
     */
    chksum_ptr = reinterpret_cast<uint32_t *>(fileBuffer);
    *chksum_ptr = ~(*chksum_ptr) & 0xffffffff;

    writeSuperblockFromBuffer(path);
}


void
SmSuperblockTestDriver::corruptSuperblockHeader(std::string& path)
{
    fds_verify(NULL == fileBuffer);

    /* offset(4) => offset(512) contains the header file.  But,
     * the meaningful data resize in offset(4) => offset(32).
     * So, corrupt a byte of header between the range.
     */
    readSuperblockToBuffer(path);
    corruptByte(sizeof(uint32_t),
                offsetof(struct SmSuperblockHeader, SmSbHdrLastFieldDummy));

    writeSuperblockFromBuffer(path);
}

void
SmSuperblockTestDriver::corruptSuperblockChangeVersion(std::string& path)
{
    fds_verify(NULL == fileBuffer);
    uint16_t *major_ptr;
    uint16_t *minor_ptr;

    readSuperblockToBuffer(path);

    major_ptr = reinterpret_cast<uint16_t *>(&fileBuffer[offsetof(struct SmSuperblockHeader, SmSbHdrMajorVer)]);

    minor_ptr = reinterpret_cast<uint16_t *>(&fileBuffer[offsetof(struct SmSuperblockHeader, SmSbHdrMinorVer)]);
    *major_ptr = ~(*major_ptr) & 0xffff;
    *minor_ptr = ~(*minor_ptr) & 0xffff;

    writeSuperblockFromBuffer(path);

}

void
SmSuperblockTestDriver::corruptSuperblockOLT(std::string& path)
{
    fds_verify(NULL == fileBuffer);

    /* OLT is from offset(512) to offset(1536), where size==1024 bytes.
     * Corrupt somwhere early in the table, so we don't it's easier to
     * detect.
     */

    readSuperblockToBuffer(path);

    corruptByte(sizeof(struct SmSuperblockHeader),
                sizeof(struct ObjectLocationTable) - sizeof(uint8_t));

    writeSuperblockFromBuffer(path);
}

void
SmSuperblockTestDriver::corruptSuperblockAnywhere(std::string& path)
{
    fds_verify(NULL == fileBuffer);

    /* Corrupt anywhere in the superblock
     */
    readSuperblockToBuffer(path);

    corruptByte(0, sizeof(struct SmSuperblock) - sizeof(uint8_t));

    writeSuperblockFromBuffer(path);
}

void
SmSuperblockTestDriver::removeSuperblock(std::string& path)
{
    int ret = remove(path.c_str());
    fds_verify(0 == ret);
}

void
SmSuperblockTestDriver::loadSuperblock()
{
    sblock->loadSuperblock(hdds, ssds, diskMap);
    GLOGNORMAL << *sblock;
}

/*
 * Sync the SM superblock to all disks.
 */
void
SmSuperblockTestDriver::syncSuperblock()
{
    sblock->syncSuperblock();
}

Error
SmSuperblockTestDriver::setDLTVersion(fds_uint64_t dltVersion, bool syncNow)
{
    return sblock->setDLTVersion(dltVersion, syncNow);
}

fds_uint64_t
SmSuperblockTestDriver::getDLTVersion()
{
    return sblock->getDLTVersion();
}


bool
SmSuperblockTestDriver::compareFiles(const std::string& filePath1,
                                     const std::string& filePath2)
{
    std::ifstream f1(filePath1.c_str());
    std::ifstream f2(filePath2.c_str());

    if (std::equal(std::istreambuf_iterator<char>(f1),
                   std::istreambuf_iterator<char>(),
                   std::istreambuf_iterator<char>(f2))) {
        return true;
    } else {
        return false;
    }
}

SmTokenSet
SmSuperblockTestDriver::getSmTokens(fds_uint16_t diskId) {
    return sblock->superblockMaster.olt.getSmTokens(diskId);
}

bool
SmSuperblockTestDriver::verifyOLT()
{
    LOGNORMAL << "HDDs size=" << hdds.size();
    LOGNORMAL << "SSDs size=" << ssds.size();
    ssize_t min = 0x0fffffff;
    ssize_t max = 0;

    if (hdds.size() > 0) {
        for (auto diskId : hdds) {
            LOGNORMAL << "Disk=" << diskId;
            LOGNORMAL << sblock->superblockMaster.olt;
            SmTokenSet tmpTokens = sblock->superblockMaster.olt.getSmTokens(diskId);
            LOGNORMAL << "disk=" << diskId << ", numToken=" << tmpTokens.size();
            if ((ssize_t)tmpTokens.size() < min) {
                min = tmpTokens.size();
            }

            if ((ssize_t)tmpTokens.size() > max) {
                max = tmpTokens.size();
            }
        }

        LOGNORMAL << "HDD token ownership different=" << std::abs(max - min);
        if (std::abs(max - min) > 2) {
            return false;
        }
    }

    min = 0x0fffffff;
    max = 0;

    if (ssds.size() > 0) {
        for (auto diskId : ssds) {
            LOGNORMAL << "Disk=" << diskId;
            LOGNORMAL << sblock->superblockMaster.olt;
            SmTokenSet tmpTokens = sblock->superblockMaster.olt.getSmTokens(diskId);
            LOGNORMAL << "disk=" << diskId << ", numToken=" << tmpTokens.size();
            if ((ssize_t)tmpTokens.size() < min) {
                min = tmpTokens.size();
            }

            if ((ssize_t)tmpTokens.size() > max) {
                max = tmpTokens.size();
            }
        }

        LOGNORMAL << "SSD token ownership different=" << std::abs(max - min);
        if (std::abs(max - min) > 2) {
            return false;
        }
    }

    return true;
}

/*
 * test1:
 *
 * Create device directories and create a new set of SM superblocks.
 */
TEST(SmSuperblockTestDriver, test1)
{
    SmSuperblockTestDriver *test1 = new SmSuperblockTestDriver();

    test1->deleteDirs();
    test1->createDirs();
    test1->loadSuperblock();

    delete test1;
}

/*
 * test2:
 *
 * From existing test setup from test1, load superblocks.
 */
TEST(SmSuperblockTestDriver, test2)
{
    SmSuperblockTestDriver *test2 = new SmSuperblockTestDriver();

    test2->loadSuperblock();

    delete test2;
}


/*
 * test3:
 *
 * From existing test testup from test1 and test2, sync superblock.
 */
TEST(SmSuperblockTestDriver, test3)
{
    SmSuperblockTestDriver *test3 = new SmSuperblockTestDriver();

    test3->loadSuperblock();
    test3->syncSuperblock();

    delete test3;
}

/*
 * test4:
 *
 * Corrupt one of the superblocks and check if superblock manager
 * properly reconcile.
 */
TEST(SmSuperblockTestDriver, test4)
{
    SmSuperblockTestDriver *test4 = new SmSuperblockTestDriver();

    /* new test setup.
     */
    test4->deleteDirs();
    test4->createDirs();
    test4->loadSuperblock();

    /* choose two files: one to corrupt, and another for reference.
     */
    std::string filePath1 = test4->getDiskPath(1) +
                            "/"  +
                            test4->getSmSuperblockFileName();

    std::string filePath2 = test4->getDiskPath(2) +
                            "/"  +
                            test4->getSmSuperblockFileName();

    /* After pristine start, the file1 and file2 should be the same.
     */
    EXPECT_TRUE(test4->compareFiles(filePath1, filePath2));

    /* Corrupt file1.
     */
    test4->corruptSuperblockAnywhere(filePath1);

    /* After corruption, the file should be different.
     */
    EXPECT_FALSE(test4->compareFiles(filePath1, filePath2));

    /* Delete superblock test driver instance, so we can start a new one.
     */
    delete test4;

    /* Start a new instance of superblock test driver.
     */
    test4 = new SmSuperblockTestDriver();
    test4->loadSuperblock();

    /* Now the superblock should've reconciled, and both file should be same.
     */
    EXPECT_TRUE(test4->compareFiles(filePath1, filePath2));
}

/* test 5
 * Corrupt majority of the superblock.
 * The expectation is that the test should panic, since we don't have the
 * logic to reconcile, if majority of the superblock is corrupted.
 * This should be a very rare case.
 * Don't run this test, and have it disabled until we figure out what to
 * do.
 * To run, use "--gtest_also_run_disabled_tests" to execute this
 */
TEST(SmSuperblockTestDriver, DISABLED_test5)
{
    SmSuperblockTestDriver *test5 = new SmSuperblockTestDriver();

    /* new test setup.
     */
    test5->deleteDirs();
    test5->createDirs();
    test5->loadSuperblock();

    /* choose two files: one to corrupt, and another for reference.
     */
    std::string filePath1 = test5->getDiskPath(1) +
                            "/"  +
                            test5->getSmSuperblockFileName();

    std::string filePath2 = test5->getDiskPath(2) +
                            "/"  +
                            test5->getSmSuperblockFileName();

    std::string filePath3 = test5->getDiskPath(3) +
                            "/"  +
                            test5->getSmSuperblockFileName();

    /* After pristine start, the file1, file2, and file3 should be the same.
     */
    EXPECT_TRUE(test5->compareFiles(filePath1, filePath2));
    EXPECT_TRUE(test5->compareFiles(filePath2, filePath3));

    /* Corrupt file1.
     */
    test5->corruptSuperblockAnywhere(filePath1);
    test5->corruptSuperblockAnywhere(filePath2);

    /* After corruption, the file should be different.
     */
    EXPECT_FALSE(test5->compareFiles(filePath1, filePath3));
    EXPECT_FALSE(test5->compareFiles(filePath2, filePath3));

    /* Delete superblock test driver instance, so we can start a new one.
     */
    delete test5;

    /* Start a new instance of superblock test driver.
     */
    test5 = new SmSuperblockTestDriver();
    test5->loadSuperblock();

    /* ????
     * Should never reach here, if this test case is executed, since
     * we haven't implemented proper action behind it.
     * It should panic before reaching here.
     */
    EXPECT_TRUE(0 == 1);
    /*           \<> /
     *
     * Almost made a face with it.  ;-)
     */
}



/* test6
 *
 * Tests condition where a disk is removed from persistent table.
 */
TEST(SmSuperblockTestDriver, test6)
{
    // Default is 3 hdds in the superblock.
    SmSuperblockTestDriver *test6 = new SmSuperblockTestDriver();

    test6->deleteDirs();
    test6->createDirs();
    test6->loadSuperblock();

    delete test6;

    /* Effective remove disk 3
     */
    test6 = new SmSuperblockTestDriver(2, 0);
    test6->loadSuperblock();

    test6->syncSuperblock();

    EXPECT_TRUE(test6->verifyOLT());
}

/* test7
 *
 * Tests condition where a new disk is added between SM instances.
 */
TEST(SmSuperblockTestDriver, test7)
{
    // Default is 3 hdds in the superblock.
    SmSuperblockTestDriver *test7 = new SmSuperblockTestDriver();

    test7->deleteDirs();
    test7->createDirs();
    test7->loadSuperblock();

    delete test7;

    // new diskmap with 4 disks.  added disk 4 to the mix.
    test7 = new SmSuperblockTestDriver(4, 0);
    test7->createDirs(true);
    test7->loadSuperblock();

    // TODO(Sean):  Since we are not currently handling adding a new disk, and
    //              re-distributing levelDB and token files, no tokens should be
    //              assigned to disk 4.
    SmTokenSet tmpTokens = test7->getSmTokens(4);
    EXPECT_EQ(tmpTokens.size(), 0u);
    test7->deleteDirs();
}


/*
 * test8
 *
 * Test to check the sync'ing of DLT version stored in the SMSuperblock.
 */
TEST(SmSuperblockTestDriver, test8)
{
    SmSuperblockTestDriver *test8_1 = new SmSuperblockTestDriver();

    test8_1->deleteDirs();
    test8_1->createDirs();
    test8_1->loadSuperblock();

    /* Very first version (i.e. pristine state) has version 0. */
    EXPECT_EQ(0UL, test8_1->getDLTVersion());

    /* Set to some version and verify that it's stored properly. */
    test8_1->setDLTVersion(0xff, true);
    test8_1->syncSuperblock();

    /* Create another superblock instance to verify that the first instance
     * correctly sync'ed to disk.
     */
    SmSuperblockTestDriver *test8_2 = new SmSuperblockTestDriver();

    test8_2->loadSuperblock();
    EXPECT_EQ(test8_1->getDLTVersion(), test8_2->getDLTVersion());

    /* Set the dlt version to 1 and see if the version is updated correctly.
     */
    test8_1->setDLTVersion(1, true);
    test8_1->syncSuperblock();

    test8_2->loadSuperblock();
    EXPECT_EQ(test8_1->getDLTVersion(), test8_2->getDLTVersion());

}


}  // fds

int
main(int argc, char *argv[])
{
    /* random seed */
    std::srand(std::time(0));
    fds::SmSuperblockUtProc superblockProc(argc, argv, "platform.conf",
                                           "fds.sm.", NULL);
    ::testing::InitGoogleMock(&argc, argv);

    return RUN_ALL_TESTS();
}


