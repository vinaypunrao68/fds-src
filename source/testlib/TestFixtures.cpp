/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#define GTEST_USE_OWN_TR1_TUPLE 0

#include <string>
#include <apis/ConfigurationService.h>
#include <SvcMsgFactory.h>
#include <TestFixtures.h>

// using ::testing::AtLeast;
// using ::testing::Return;

namespace fds {

TestPlatform::TestPlatform(int argc, char *argv[], const std::string &log, Module **vec)
    : FdsService(argc, argv, log, vec)
{
}

/* Override run() to do nothing.  User of this class must start_modules() and
 * shutdown_modules() manually to start and shutdown modules
 */
int TestPlatform::run()
{
    return 0;
}

/* Static member init */
int SingleNodeTest::argc_;
char** SingleNodeTest::argv_ = nullptr;
std::unique_ptr<TestPlatform> SingleNodeTest::platform_;
std::string SingleNodeTest::volName_;
uint64_t SingleNodeTest::volId_;
std::unique_ptr<Deployer> SingleNodeTest::deployer_;

/**
* @brief 
*/
SingleNodeTest::SingleNodeTest()
{
}

/**
* @brief Initializes static members
*
* @param argc
* @param argv[]
* @param volName
*/
void SingleNodeTest::init(int argc, char *argv[], const std::string volName)
{
    argc_ = argc;
    argv_ = argv;
    volName_ = volName;
    /* TODO(Rao): Parse out fds tool dir from argv */
    deployer_.reset(new Deployer("/home/nbayyana/temp/fds-src/source/tools"));
}

/**
* @brief Sets up
* 1. Single node fds environment
* 2. Platform
*/
void SingleNodeTest::SetUpTestCase()
{
    /* TODO(Rao): Run fds start */
    bool ret = deployer_->setupOneNodeDomain();
    ASSERT_TRUE(ret == true);

    /* Start test platform */
    platform_.reset(new TestPlatform(argc_, argv_, "SingleNodeTest.log", nullptr));
    platform_->start_modules();

    /* Induce sleep before creating volume */
    sleep(5);

    /* Create a volume */
    auto omCfg = gModuleProvider->get_plf_manager()->plf_om_master()->get_om_config_svc();
    apis::VolumeSettings vs;
    ASSERT_NO_THROW(omCfg->createVolume("TestDomain", volName_,
                                        SvcMsgFactory::defaultS3VolSettings(), 0));
    ASSERT_NO_THROW(volId_ = omCfg->getVolumeId(volName_));

    /* Induce a short sleep, till volume information is propaged.  Ideally this shouldn't be
     * needed if createVolume blocks until volume information is propagated
     */
    sleep(1);
}

/**
* @brief Tears down
* 1. Single node fds environment
* 2. platform
*/
void SingleNodeTest::TearDownTestCase()
{
    std::cout << "TearDownTestCase\n";
    platform_->shutdown_modules();
    platform_.reset();

    bool ret = deployer_->tearDownOneNodeDomain(true);
    std::cout << "Teardown exit status: " << ret << std::endl;
    // TODO(Rao): Enable below when fds stop starts returning 0 as exit status
    // EXPECT_TRUE(ret == true) << "fds stop is returning status code 1 for some reason";
    deployer_.reset();
}
}  // namespace fds
