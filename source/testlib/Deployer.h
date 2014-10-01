/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_DEPLOYER_H_
#define SOURCE_INCLUDE_DEPLOYER_H_

#include <string>

namespace fds {

/**
* @brief Use this class for fds deployement
*/
struct Deployer {
    Deployer(const std::string &toolsDir);
    virtual ~Deployer();
    bool setupOneNodeDomain();
    bool tearDownOneNodeDomain(bool clean);

 protected:
    std::string toolsDir_;
};

}  // namespace fds

#endif  // SOURCE_INCLUDE_DEPLOYER_H_
