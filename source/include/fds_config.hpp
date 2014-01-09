/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef _FDS_CONFIG_H_
#define _FDS_CONFIG_H_

#include <string>
#include <exception>

#include <libconfig.h++>

namespace fds {

/**
 * Generic Fds exception
 */
class FdsException : public std::exception {
public:
    FdsException(const std::string &desc)
    {
        desc_ = desc;
    }
    const char* what() const noexcept {return desc_.c_str();}
private:
    std::string desc_;
};

/**
 *
 */
class FdsConfig {
public:
    FdsConfig(const std::string &config_file)
    {
        config_.readFile(config_file.c_str());
    }

    template<class T>
    T get(const std::string &key);

    template<class T>
    T get(const std::string &key, const T &default_value);

private:
    /* Config object */
    libconfig::Config config_;
};

/**
 *
 * @param key key is the path of the key to get.  Key is '.' separated.
 * @return value associated with key.  If key doesn't exist throws an exception
 */
template<class T>
T FdsConfig::get(const std::string &key)
{
    T val;
    if (!config_.lookupValue(key, val)) {
        throw FdsException("Failed to lookup " + key);
    }
    return val;

}

/**
 *
 * @param key key key is the path of the key to get.  Key is '.' separated.
 * @param default_value In case the key isn't found, default_value is returned
 * @return value associated with key or default_value when key doesn't exist
 */
template<class T>
T FdsConfig::get(const std::string &key, const T &default_value)
{
    T val;
    if (!config_.lookupValue(key, val)) {
        return default_value;
    }
    return val;
}

} // namespace Fds

#endif
