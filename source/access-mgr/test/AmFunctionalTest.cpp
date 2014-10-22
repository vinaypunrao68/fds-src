/*
 * Copyright 2013 Formation Data Systems, Inc.
 */
#define GTEST_USE_OWN_TR1_TUPLE 0

#include <string>
#include <vector>
#include <fdsn-server.h>
#include <util/fds_stat.h>
#include <native_api.h>
#include <am-platform.h>
#include <net/net-service.h>
#include <AccessMgr.h>

#include <gtest/gtest.h>

namespace fds {

AccessMgr::unique_ptr am;

class AmLoadProc : public FdsProcess {
  public:
    AmLoadProc(int argc, char * argv[], const std::string & config,
               const std::string & basePath, Module * vec[])
            : FdsProcess(argc, argv, config, basePath, vec) {
        am = AccessMgr::unique_ptr(new AccessMgr("AM Functional Test Module",
                                                 this));
        dataApi = am->dataApi;
    }
    virtual ~AmLoadProc() {
    }

    virtual int run() override {
        // register and populate volumes
        VolumeDesc volDesc("Test Volume", 5);
        fds_verify(am->registerVolume(volDesc) == ERR_OK);

        return 0;
    }

  private:
    AmDataApi::shared_ptr dataApi;
};

TEST(AccessMgr, statBlob) {
    GLOGDEBUG << "Testing stat blob";
    apis::BlobDescriptor blobDesc;
    boost::shared_ptr<std::string> domainName(
        boost::make_shared<std::string>("Test Domain"));
    boost::shared_ptr<std::string> volumeName(
        boost::make_shared<std::string>("Test Volume"));
    boost::shared_ptr<std::string> blobName(
        boost::make_shared<std::string>("Test Blob"));
    am->dataApi->statBlob(blobDesc, domainName, volumeName, blobName);
}

}  // namespace fds

int
main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    fds::Module *amVec[] = {
        nullptr
    };
    fds::AmLoadProc ap(argc, argv, "platform.conf", "fds.am.", amVec);
    ap.main();

    return RUN_ALL_TESTS();
}
