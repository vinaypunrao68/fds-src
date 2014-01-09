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
 * Helper class for accessing config values from FdsConfig.  It essentially
 * stores the FdsConfig object and base path with in the config.  For every
 * get(key) access basepath is appened to the key.
 */
class FdsConfigAccessor {
public:
    /**
     *
     * @param fds_config FdsConfig object
     * @param base_path base path
     */
    FdsConfigAccessor(const boost::shared_ptr<FdsConfig> &fds_config,
            const std::string &base_path)
    {
        fds_config_ = fds_config;
        base_path_ = base_path;
    }

    /**
     *
     * @return FdsConfig object
     */
    boost::shared_ptr<FdsConfig> get_fds_config()
    {
        return fds_config_;
    }

    /**
     *
     * @return base path
     */
    std::string get_base_path()
    {
        return base_path_;
    }

    /**
     * @see FdsConfig::get(const std::string &key)
     * @param key - config key without the basepath
     * @return
     */
    template<class T>
    T get(const std::string &key)
    {
        return fds_config_->get<T>(base_path_+key);
    }

    /**
     * @see FdsConfig::get(const std::string &key, const T &default_value)
     * @param key - config key without the basepath
     * @param default_value
     * @return
     */
    template<class T>
    T get(const std::string &key, const T &default_value)
    {
        return fds_config_->get<T>(base_path_+key, default_value);
    }

private:
    boost::shared_ptr<FdsConfig> fds_config_;
    std::string base_path_;
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
