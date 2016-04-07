/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#ifndef SOURCE_PLATFORM_INCLUDE_PLATFORM_ENVIRONMENT_H_
#define SOURCE_PLATFORM_INCLUDE_PLATFORM_ENVIRONMENT_H_

#include <vector>
#include <unordered_map>

namespace fds {

typedef std::unordered_map<std::string, std::string> EnvironmentMap;
typedef std::unordered_map<int, EnvironmentMap> ServiceEnvironments;

class Environment {
 public:
    static void initialize();
    static void setEnvironment(int service);

private:
    // helper for initialize
    static void ingest(int idx, std::string str);

    // helper to lookup a service in the singleton
    static EnvironmentMap& getEnvironmentForService(int service);

    // core function for setting environment variables in the process
    static void setEnvironment(EnvironmentMap& env);

    //the singleton
    static Environment* e;
    static Environment* getEnv() {
        if (!e) {
            e = new Environment;
        }
        return e;
    }

    // the instatiation of the singleton
    Environment(){};
    ServiceEnvironments envs;

};

}  // namespace fds

#endif  // SOURCE_PLATFORM_INCLUDE_PLATFORM_ENVIRONMENT_H_
