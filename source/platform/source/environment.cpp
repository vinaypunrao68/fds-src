/*
 * Copyright 2016 Formation Data Systems, Inc.
 */

#include <platform/environment.h>
#include <boost/tokenizer.hpp>

// this is just to get the Enum for Services
#include <fdsp/health_monitoring_api_types.h>
#include <platform/platform_manager.h>

namespace fds {


Environment* Environment::e = 0;

void Environment::initialize() {
    GLOGDEBUG << "Initializing environment variable configurations";

    std::string temp;
    std::string dummy("");

    temp = CONFIG_STR("fds.pm.environment.am", dummy);
    if (temp.size() != 0) {
        ingest(pm::BARE_AM, temp);
    }

    temp = CONFIG_STR("fds.pm.environment.dm", dummy);
    if (temp.size() != 0) {
        ingest(pm::DATA_MANAGER, temp);
    }

    temp = CONFIG_STR("fds.pm.environment.sm", dummy);
    if (temp.size() != 0) {
        ingest(pm::STORAGE_MANAGER, temp);
    }

    temp = CONFIG_STR("fds.pm.environment.xdi", dummy);
    if (temp.size() != 0) {
        ingest(pm::JAVA_AM, temp);
    }

}

/* takes a sting of the form "key1=val1;key2=val2;..." and produces a map with pairs (key1, val1) (key2, val2) ...*/
void Environment::ingest(int idx, std::string str){
    EnvironmentMap& env = getEnvironmentForService(idx);

    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    boost::char_separator<char> sep(";=");

    tokenizer tokens(str, sep);

    for (tokenizer::iterator tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter) {
        auto prev = *tok_iter;
        ++tok_iter;
        if (tok_iter != tokens.end()) {
            env[prev] = *tok_iter;
        }
    }
}

EnvironmentMap& Environment::getEnvironmentForService(int service) {
    return getEnv()->envs[service];
}

// had to remove logging form this function because it is called after PM forks and ends up making a new log file
void Environment::setEnvironment(EnvironmentMap& env) {
    if (env.size() != 0) {
        for(auto i : env) {
            setenv(i.first.c_str(), i.second.c_str(), 1);
        }
    }
}

void Environment::setEnvironment(int service) {
    setEnvironment(getEnvironmentForService(service));
}

}  // namespace fds
