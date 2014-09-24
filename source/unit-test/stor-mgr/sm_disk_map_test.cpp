/**
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <unistd.h>

#include <vector>
#include <chrono>
#include <thread>
#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>

#include <util/Log.h>
#include <fds_assert.h>
#include <fds_process.h>
#include <ObjectId.h>
#include <FdsRandom.h>
#include <object-store/SmDiskMap.h>

namespace fds {

/**
 * Unit test for SmDiskMap class
 */
class SmDiskMapUtProc : public FdsProcess {
  public:
    SmDiskMapUtProc(int argc, char * argv[], const std::string & config,
                    const std::string & basePath, Module * vec[]);
    virtual ~SmDiskMapUtProc();

    virtual int run() override;

  private:
    /* helper methods */
    void populateDlt(DLT* dlt);

    fds::SmDiskMap::unique_ptr smDiskMap;
};

SmDiskMapUtProc::SmDiskMapUtProc(int argc, char * argv[], const std::string & config,
                                 const std::string & basePath, Module * vec[]) {
    smDiskMap = fds::SmDiskMap::unique_ptr(
        new fds::SmDiskMap("Test SM Disk Map"));
    std::cout << "Will test Sm Disk Map" << std::endl;

    fds::Module *smVec[] = {
        &diskio::gl_dataIOMod,
        smDiskMap.get(),
        nullptr
    };

    init(argc, argv, config, basePath, "smdiskmap_ut.log", vec);
}

SmDiskMapUtProc::~SmDiskMapUtProc() {
}

void SmDiskMapUtProc::populateDlt(DLT* dlt) {
}

int SmDiskMapUtProc::run() {
    Error err(ERR_OK);
    int ret = 0;
    std::cout << "Starting test...";

    DLT* dlt = new DLT(16, 4, 1, true);
    smDiskMap->handleNewDlt(dlt);

    // we don't need dlt anymore
    delete dlt;

    if (ret == 0) {
        std::cout << "Test PASSED" << std::endl;
    } else {
        std::cout << "Test FAILED" << std::endl;
    }
    return ret;
}
}  // namespace fds

int main(int argc, char * argv[]) {
    fds::SmDiskMapUtProc p(argc, argv, "sm_ut.conf", "fds.diskmap_ut.", NULL);

    std::cout << "unit test " << __FILE__ << " started." << std::endl;
    p.main();
    std::cout << "unit test " << __FILE__ << " finished." << std::endl;
    return 0;
}

