/**
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <ostream>
#include <string>

#include <fds_process.h>
#include <object-store/SmSuperblock.h>

namespace fds {


/* TODO(Sean)
 *      Need to convert the test to gtest format, but for now I am hacking
 *      it like a traditional unit test.
 */
class SmSuperblockTestDriver {
  public:
    SmSuperblockTestDriver();
    ~SmSuperblockTestDriver();

    void createDirs();
    void deleteDirs();
    void loadSuperblock();
    void syncSuperblock();

  private:
    DiskLocMap diskMap = {{0,"/tmp/hddA/"}, {1,"/tmp/hddB/"}, {2,"/tmp/hddC/"}};
    SmSuperblockMgr *sblock;
};

SmSuperblockTestDriver::SmSuperblockTestDriver()
{
    sblock = new SmSuperblockMgr();
    // setDiskMap();
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

    std::cout << "executing " << __func__ << std::endl;

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
        std::cout << "Creating disk(" << cit->first << ") => " << dirPath << std::endl;
        if (mkdir(dirPath.c_str(), 0777) != 0) {
            /* If directory exists, then likely the superblock file exists.
             * Delete them to make it a pristine state.
             */
            if (errno == EEXIST) {
                std::cout << "Directory "<< dirPath << "already exists" << std::endl;
                std::cout << "Trying to removing file: " << filePath << std::endl;
                if (remove(filePath.c_str()) == 0) {
                    std::cout << "Successfully removed file: " << filePath << std::endl;
                } else {
                    std::cout << "Couldn't locate file: " << filePath << std::endl;
                }

            } else if (errno == EACCES) {
                std::cout << "Access denied (" << dirPath << std::endl;
            }
        }
    }

    std::cout << "completing " << __func__ << std::endl;
}

void
SmSuperblockTestDriver::deleteDirs()
{
    std::string dirPath;
    std::string filePath;

    std::cout << "executing " << __func__ << std::endl;

    for (DiskLocMap::const_iterator cit = diskMap.cbegin();
         cit != diskMap.cend();
         ++cit) {
        dirPath = cit->second;
        filePath = dirPath;
        filePath.append(sblock->SmSuperblockMgrTestGetFileName());

        /* remove file and directory.  ignore return values.
         */
        if (remove(filePath.c_str()) == 0) {
            std::cout << "Successfully removed file: " << filePath << std::endl;
        } else {
            std::cout << "Couldn't remove file: " << filePath << std::endl;
        }
        if (remove(dirPath.c_str()) == 0) {
            std::cout << "Successfully removed dir: " << dirPath << std::endl;
        } else {
            std::cout << "Couldn't remove file: " << dirPath << std::endl;
        }
    }

    std::cout << "completing " << __func__ << std::endl;

}

void
SmSuperblockTestDriver::loadSuperblock()
{
    std::cout << "Loading Superblock" << std::endl;

    sblock->loadSuperblock(diskMap);
}

void
SmSuperblockTestDriver::syncSuperblock()
{
    std::cout << "Syncing Superblock" << std::endl;

    sblock->syncSuperblock();
}


void
test1()
{
    std::cout << "executing " << __func__ << std::endl;
    SmSuperblockTestDriver *test1 = new SmSuperblockTestDriver();

    test1->createDirs();
    test1->loadSuperblock();

    std::cout << "completing " << __func__ << std::endl;
}

void
test2()
{
    std::cout << "executing " << __func__ << std::endl;
    SmSuperblockTestDriver *test1 = new SmSuperblockTestDriver();

    test1->loadSuperblock();

    std::cout << "completing " << __func__ << std::endl;
}

void
test3()
{
    std::cout << "executing " << __func__ << std::endl;
    SmSuperblockTestDriver *test1 = new SmSuperblockTestDriver();

    test1->syncSuperblock();

    std::cout << "completing " << __func__ << std::endl;
}



}  // fds

int
main(int argc, char *argv[])
{
    fds::test1();

    fds::test2();

    fds::test3();

    std::cout << "completion" << std::endl;

    return 0;
}



