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

/* takes a string of the form "key1=val1;key2=val2;..." and produces a map with pairs (key1, val1) (key2, val2) ...*/
void Environment::ingest(int idx, std::string str){
    EnvironmentMap& env = getEnvironmentForService(idx);

    typedef boost::tokenizer<boost::char_separator<char> > tokenizer;
    boost::char_separator<char> sep(";=");

    tokenizer tokens(str, sep);

    for (tokenizer::iterator tok_iter = tokens.begin(); tok_iter != tokens.end(); ++tok_iter) {
        auto prev = *tok_iter;
        ++tok_iter;
        if (tok_iter != tokens.end()) {
            /* A hack for tcmalloc. set the high bit of the first byte to cause tcmalloc to append pid to profile file name.
               otherwise we overwrite files when OOM killer forces a process to exit */
            if (prev == "HEAPPROFILE") {
                char* tmp = (char*)malloc(tok_iter->size()+1);

                strncpy(tmp, tok_iter->c_str(), tok_iter->size()+1);

                tmp[0] |= 128;

                env.emplace(prev, std::string(tmp));

                free(tmp);
            }else{
                env[prev] = *tok_iter;
            }
        }
    }
}

EnvironmentMap& Environment::getEnvironmentForService(int service) {
    return getEnv()->envs[service];
}

/* had to remove happy path logging from this function because it is called after PM forks and ends up making a new log file,
   which breaks system tests that look for PM log messages in the latest file only. */
void Environment::setEnvironment(EnvironmentMap& env) {
    if (env.size() != 0) {
        for(auto i : env) {
            auto err = setenv(i.first.c_str(), i.second.c_str(), 1);

            if (err) {
                // since this is called after forking, message will end up in an incremented log file
                GLOGDEBUG << "Failed setting environment variable " << i.first << "=" << i.second;
            }
        }
    }
}

void Environment::setEnvironment(int service) {
    setEnvironment(getEnvironmentForService(service));
}

}  // namespace fds
