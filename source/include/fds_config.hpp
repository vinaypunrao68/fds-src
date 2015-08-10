/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef _FDS_CONFIG_H_
#define _FDS_CONFIG_H_

#include <string>
#include <exception>
#include <iostream>
#include <concurrency/Mutex.h>
#include <libconfig.h++>
#include <fds_error.h>
#include <sys/stat.h>

namespace fds {
/**
 *
 */
class FdsConfig {
  public:

    FdsConfig();

    FdsConfig(const std::string &default_config_file, int argc, char *argv[]);

    /**
     * Parses command line arguments and overrides the config object
     * with them
     * @param default_config_file - deafult confilg file path.  Can be over
     * ridden on command line
     * @param argc
     * @param agrv
     */
    void init(const std::string &default_config_file, int argc, char* argv[]);
    /**
     * Return true if key exists in the config
     * @param key
     */
    bool exists(const std::string &key);

    template<class T> T get(const std::string &key) {
        T val;
        fds_mutex::scoped_lock l(lock_);
        if (!config_.lookupValue(key, val)) {
            throw fds::Exception("Failed to lookup " + key);
        }
        return val;
    }
    
    template<class T> 
    T get(const std::string &key, const T &default_value) {
        if (!exists(key)) return default_value;
        try {
            return get<T>(key);
        } catch (fds::Exception const& e) {
        }
        return default_value;
    }

    const libconfig::Config& getConfig() const {
        return config_;
    }

    void set(const std::string &key, const char *value);
    void set(const std::string &key, const std::string& value);
    void set(const std::string &key, const fds_int32_t& value);
    void set(const std::string &key, const fds_uint32_t& value);
    void set(const std::string &key, const fds_bool_t& value);
    void set(const std::string &key, const fds_int64_t& value);
    void set(const std::string &key, const fds_uint64_t& value);
    void set(const std::string &key, const double& value);
    
  private:
    /* Config object */
    fds_mutex lock_;
    libconfig::Config config_;
};
typedef boost::shared_ptr<FdsConfig> FdsConfigPtr;

template<> std::string  FdsConfig::get<std::string> (const std::string &key);
template<> fds_int32_t  FdsConfig::get<fds_int32_t> (const std::string &key);
template<> fds_uint32_t FdsConfig::get<fds_uint32_t>(const std::string &key);
template<> fds_bool_t   FdsConfig::get<fds_bool_t>  (const std::string &key);
template<> fds_int64_t  FdsConfig::get<fds_int64_t>(const std::string &key);
template<> fds_uint64_t FdsConfig::get<fds_uint64_t>(const std::string &key);
template<> double       FdsConfig::get<double>      (const std::string &key);

/**
 * Helper class for accessing config values from FdsConfig.  It essentially
 * stores the FdsConfig object and base path with in the config.  For every
 * get(key) access basepath is appened to the key.
 */
class FdsConfigAccessor {
  public:
    /**
     * Default constructor.  Follow it up with init()
     */
    FdsConfigAccessor();

    /**
     *
     * @param fds_config FdsConfig object
     * @param base_path base path
     */
    FdsConfigAccessor(const boost::shared_ptr<FdsConfig> &fds_config, const std::string &base_path);

    /**
     *
     * @param fds_config FdsConfig object
     * @param base_path base path
     */
    void init(const boost::shared_ptr<FdsConfig> &fds_config, const std::string &base_path);
    /**
     *
     * @return FdsConfig object
     */
    const boost::shared_ptr<FdsConfig> get_fds_config() const;

    /**
     *
     * @return base path
     */
    std::string get_base_path();
    void set_base_path(const std::string &base);

    /**
     * @see FdsConfig::get(const std::string &key)
     * @param key - config key without the basepath
     * @return
     */
    template<class T>
    T get(const std::string &key) {
        return fds_config_->get<T>(base_path_+key);
    }

    /**
     * @see FdsConfig::get(const std::string &key, const T &default_value)
     * @param key - config key without the basepath
     * @param default_value
     * @return
     */
    template<class T>
    T get(const std::string &key, const T &default_value) {
        return fds_config_->get<T>(base_path_+key, default_value);
    }

    /**
     * @see FdsConfig::get(const std::string &key)
     * @param key - config key with the basepath
     * @return
     */
    template<class T>
    T get_abs(const std::string &key) {
        return fds_config_->get<T>(key);
    }

    /**
     * @see FdsConfig::get(const std::string &key, const T &default_value)
     * @param key - config key with the basepath
     * @param default_value
     * @return
     */
    template<class T>
    T get_abs(const std::string &key, const T &default_value) {
        return fds_config_->get<T>(key, default_value);
    }

    /**
     * Return true if key exists in the config
     * @param key
     */
    bool exists(const std::string &key);

  private:
    boost::shared_ptr<FdsConfig> fds_config_;
    std::string base_path_;
};

} // namespace Fds

#endif
