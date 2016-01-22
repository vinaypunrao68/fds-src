/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#define GTEST_USE_OWN_TR1_TUPLE 0

#include <string>
#include <fdsp/ConfigurationService.h>
#include <SvcMsgFactory.h>
#include <TestFixtures.h>
#include <util/stringutils.h>

#include "platform/platform.h"

// using ::testing::AtLeast;
// using ::testing::Return;

namespace fds {

/* Static members */
int BaseTestFixture::argc_;
char** BaseTestFixture::argv_ = nullptr;
po::variables_map BaseTestFixture::vm_;

BaseTestFixture::BaseTestFixture()
{
}

void BaseTestFixture::init(int argc, char *argv[],
                           po::options_description optDesc)
{
    argc_ = argc;
    argv_ = argv;
    po::store(po::command_line_parser(argc, argv).options(optDesc).allow_unregistered().run(), vm_);
    po::notify(vm_);

    if (vm_.count("help")) {
        std::cout << optDesc << std::endl;
    }
}

void BaseTestFixture::SetUpTestCase()
{
}

void BaseTestFixture::TearDownTestCase()
{
}

void BaseTestFixture::setupDmClusterEnv(const std::string &fdsSrcDir,
                                        const std::string &baseDirPrefix)
{
    int ret = 0;
    /* Remove node[1-4] */
    for (int i = 1; i <= 4; i++) {
        ret = std::system(util::strformat("rm -rf %s/node%d/*",
                                          baseDirPrefix.c_str(), i).c_str());
        ret = std::system(util::strformat("mkdir -p %s/node%d",
                                          baseDirPrefix.c_str(), i).c_str());
    }
    /* Create node 1*/
    std::string confFilePath = util::strformat("%s/config/etc/platform.conf", fdsSrcDir.c_str());
    ret = std::system(util::strformat("mkdir -p %s/node1/etc", baseDirPrefix.c_str()).c_str());
    ret = std::system(util::strformat("cp %s %s/node1/etc/platform.conf",
                                      confFilePath.c_str(), baseDirPrefix.c_str()).c_str());
    ret = std::system(util::strformat("mkdir -p %s/node1/var/logs",
                                      baseDirPrefix.c_str()).c_str());
    ret = std::system(util::strformat("mkdir -p %s/node1/sys-repo",
                                      baseDirPrefix.c_str()).c_str());
    ret = std::system(util::strformat("mkdir -p %s/node1/user-repo",
                                      baseDirPrefix.c_str()).c_str());

    /* Copy node1 to node[2-4] */
    for (int i = 2; i <= 4; i++) {
        ret = std::system(util::strformat("cp -r %s/node1/* %s/node%d/",
                                          baseDirPrefix.c_str(),
                                          baseDirPrefix.c_str(),
                                          i).c_str());
    }
}

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
void SingleNodeTest::init(int argc, char *argv[],
                          po::options_description optDesc,
                          const std::string &volName)
{
    /* Add fds-tools as an option that must be set */
    optDesc.add_options()
        ("fds-tools", po::value<std::string>(), "tools dir");
    optDesc.add_options()
        ("deploy", po::value<bool>()->default_value(true), "bring up singlenode domain");

    /* Init base test fixture class */
    BaseTestFixture::init(argc, argv, optDesc);
    /* If help is set in cmd args return immmediately */
    if (vm_.count("help")) {
        return;
    }
    ASSERT_NO_THROW(BaseTestFixture::getArg<std::string>("fds-tools"))
        << "fds-tool directory not set";

    /* Create deployer object */
    volName_ = volName;

    /* Create a deployer only if deploy option is set */
    if (BaseTestFixture::getArg<bool>("deploy")) {
        deployer_.reset(new Deployer(BaseTestFixture::getArg<std::string>("fds-tools")));
    }
}

/**
* @brief Sets up
* 1. Single node fds environment
* 2. Platform
*/
void SingleNodeTest::SetUpTestCase()
{
    BaseTestFixture::SetUpTestCase();

    /* Only deploy, if deploy option was set */
    if (deployer_) {
        bool ret = deployer_->setupOneNodeDomain();
        ASSERT_TRUE(ret == true);
    }

    /* Start test platform */
    platform_.reset(new TestPlatform(argc_, argv_, "SingleNodeTest.log", nullptr));
    platform_->start_modules();

    /* Induce sleep before creating volume */
    /* TODO(Rao): We are inducing sleep to wait for volume and dlt propagation
     * to happen.  This is a total hack.  Our domain should handle these cases
     * gracefully
     */
    sleep(12);
    std::ostringstream oss;
    unsigned int seed = time(NULL);
    oss << rand_r(&seed);
    volName_ = volName_ + oss.str();
    // TODO(Rao): This code needs to be ported based on service map
#if 0
    /* Create a volume */
    auto omCfg = MODULEPROVIDER()->get_plf_manager()->plf_om_master()->get_om_config_svc();
    apis::VolumeSettings vs;
    ASSERT_NO_THROW(omCfg->createVolume("TestDomain", volName_,
                                        SvcMsgFactory::defaultS3VolSettings(), 0));
    ASSERT_NO_THROW(volId_ = omCfg->getVolumeId(volName_));
#endif
    if (volId_ == 0) {
        volId_ = 1;
    }
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

    if (deployer_) {
        bool ret = deployer_->tearDownOneNodeDomain(true);
        std::cout << "Teardown exit status: " << ret << std::endl;
        // TODO(Rao): Enable below when fds stop starts returning 0 as exit status
        // EXPECT_TRUE(ret == true) << "fds stop is returning status code 1 for some reason";
        deployer_.reset();
    }

    BaseTestFixture::TearDownTestCase();
}
}  // namespace fds
