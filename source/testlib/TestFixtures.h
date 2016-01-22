/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_TESTFIXTURES_H_
#define SOURCE_INCLUDE_TESTFIXTURES_H_

#include <memory>
#include <string>
#include <testlib/Deployer.h>
#include <gtest/gtest.h>

#include "platform/fds_service.h"
#include <boost/program_options.hpp>
namespace fds {

/* Forward declarations */
namespace po = boost::program_options;

/**
* @brief Base class for gTest based test fixutures.  Can be used for non-gtest based as well
*/
struct BaseTestFixture : ::testing::Test
{
    BaseTestFixture();

    static void init(int argc, char *argv[], po::options_description optDesc);

    static void SetUpTestCase();

    static void TearDownTestCase();

    /**
    * @brief Sets up a four node dm cluster environment
    *
    * @param fdsSrcDir
    * @param baseDirPrefix
    */
    static void setupDmClusterEnv(const std::string &fdsSrcDir,
                                  const std::string &baseDirPrefix);

    template <class T>
    static T getArg(const std::string& k, T defaultVal)
    {
        if (vm_.count(k)) {
            return vm_[k].as<T>();
        } else {
            return defaultVal;
        }
    }

    template <class T>
    static T getArg(const std::string& k)
    {
        if (!vm_.count(k)) {
            throw std::runtime_error(k + " doesn't exist");
        } else {
            return vm_[k].as<T>();
        }
    }

 protected:
    static int argc_;
    static char **argv_;
    /* Stores command line arguments */
    static po::variables_map vm_;
};

/**
* @brief Test platform.  It bundles platform deamon and disables the run() method.
* Using this class provides a platform deamon that can be used for testing.
*/
struct TestPlatform : FdsService
{
    TestPlatform(int argc, char *argv[],
                 const std::string &log, Module **vec);
    virtual int run() override;
};

/**
* @brief Provides Test fixture for single Node base testing.  As part of fixture setup
* following is provided.
* 1. Single node domain environemt
* 2. Test platformd setup
* 3. One S3 volume
* All the above objects that provide above functionality are provided as static members.
*
* Usage:
* 1. Derive a fixture from this class.  You will have access test platform daemon and single node
* fds environemnt will be setup for you.
* 2. In your TEST_F() tests, you can use static members such as platform, volume id, etc.
*/
struct SingleNodeTest : BaseTestFixture
{
    SingleNodeTest();
    static void init(int argc, char *argv[],
                     po::options_description optDesc,
                     const std::string &volName);

    static void SetUpTestCase();

    static void TearDownTestCase();

  protected:
    static std::unique_ptr<TestPlatform> platform_;
    static std::string volName_;
    static uint64_t volId_;
    static std::unique_ptr<Deployer> deployer_;
};
}  // namespace fds

#endif  // SOURCE_INCLUDE_TESTFIXTURES_H_
