/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <exception>
#include <string>

#include "fds_config.hpp"

namespace fds {

/**
 * Constructor
 * @param config_file file containing config information
 */
FdsConfig::FdsConfig(const std::string &config_file)
{
    config_.readFile(config_file.c_str());
}

/**
 *
 * @param key key is the path of the key to get.  Key is '.' separated.
 * @return value associated with key.  If key doesn't exist throws an exception
 */
template<class T>
T FdsConfig::get(const std::string &key) const
{
    try {
        T val;
        config_.lookup(key, val);
        return val;
    } catch(const std::exception &e) {
        throw std::exception(e.what());
    }
}

/**
 *
 * @param key key key is the path of the key to get.  Key is '.' separated.
 * @param default_value In case the key isn't found, default_value is returned
 * @return value associated with key or default_value when key doesn't exist
 */
template<class T>
T FdsConfig::get(const std::string &key, const T &default_value) const
{
    try {
        T val;
        config_.lookup(key, val);
        return val;
    } catch(const std::exception &e) {
        return default_value;
    }
}

}  // namespace fds
