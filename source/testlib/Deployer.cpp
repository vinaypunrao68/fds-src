/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <string>
#include <Deployer.h>

namespace fds {
/**
* @brief
* @param toolsDir Path to tools directory where deployment scripts are located

*/
Deployer::Deployer(const std::string &toolsDir)
{
    toolsDir_ = toolsDir;
}

/*
 * Destructor
 */
Deployer::~Deployer()
{
}

/**
* @brief Sets up one node domain using fds script
*
* @return true if exit status is 0
*/
bool Deployer::setupOneNodeDomain()
{
    auto ret = std::system((toolsDir_ + "/fds start").c_str());
    return ret == 0;
}

/**
* @brief Tears down one node domain using fds script
*
* @param clean whether to run fds clean as well
*
* @return true if exit code is zero
*/
bool Deployer::tearDownOneNodeDomain(bool clean)
{
    int stopRet = std::system((toolsDir_ + "/fds stop").c_str());
    int cleanRet = 0;
    if (clean) {
        cleanRet = std::system((toolsDir_ + "/fds clean").c_str());
    }
    return stopRet == 0 && cleanRet == 0;
}
}  // namespace fds
