/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef _FDS_CONFIG_H_
#define _FDS_CONFIG_H_

#include <string>
#include <exception>
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <iostream>
#include <concurrency/Mutex.h>

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

    FdsConfig()
    : lock_("FdsConfig")
    {
    }

    FdsConfig(const std::string &default_config_file, 
              int argc, char *argv[])
    : lock_("FdsConfig")
    {
        init(default_config_file, argc, argv);
    }

    /**
     * Parses command line arguments and overrides the config object
     * with them
     * @param default_config_file - deafult confilg file path.  Can be over
     * ridden on command line
     * @param argc
     * @param agrv
     */
    void init(const std::string &default_config_file, 
              int argc, char* argv[])
    {
        namespace po = boost::program_options;
        po::options_description desc("Allowed options");

        po::parsed_options parsed = 
                po::command_line_parser(argc, argv).options(desc).allow_unregistered().run();
        
        /* Config path specified on the command line is given preference */
        std::string config_file = default_config_file;
        for (auto o : parsed.options) {
            if (o.string_key.find("conf_file") == 0) {
                config_file = o.value[0];
                break;
            }
        }
        config_.readFile(config_file.c_str());

        /* Override config read from with command line params */
        for (auto o : parsed.options) {
            /* Ignore options that don't start with "fds." */
            if (o.string_key.find("fds.") != 0) {
                continue;
            }
            /* Look up the option key in config file.  Option must
             * exists in the config file
             */
            libconfig::Setting &s = config_.lookup(o.string_key);
            std::string temp;
            switch (s.getType()) {
                case libconfig::Setting::TypeInt64:
                    s = boost::lexical_cast<long long>(o.value[0]);
                    break;
                case libconfig::Setting::TypeFloat:
                    s = boost::lexical_cast<double>(o.value[0]);
                    break;
                case libconfig::Setting::TypeInt:
                    s = boost::lexical_cast<int>(o.value[0]);
                    break;

                case libconfig::Setting::TypeString:
                    s = o.value[0];
                    break;

                case libconfig::Setting::TypeBoolean:
                    s = boost::iequals(o.value[0], "true");
                    break;
                default:
                    throw FdsException("Unsupported type specified for key: " + o.string_key);
            }
            std::cout << o.string_key << " Overridden to " << o.value[0] << std::endl;
        }

    }

    /**
     * Return true if key exists in the config
     * @param key
     */
    bool exists(const std::string &key)
    {
        fds_spinlock::scoped_lock l(lock_);
        return config_.exists(key);
    }

    template<class T>
    T get(const std::string &key);

    template<class T>
    T get(const std::string &key, const T &default_value);

    template<class T>
    void set(const std::string &key, const T &default_value);

private:
    /* Config object */
    fds_spinlock lock_;
    libconfig::Config config_;
};
typedef boost::shared_ptr<FdsConfig> FdsConfigPtr;

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
    FdsConfigAccessor()
    {
    }

    /**
     *
     * @param fds_config FdsConfig object
     * @param base_path base path
     */
    FdsConfigAccessor(const boost::shared_ptr<FdsConfig> &fds_config,
            const std::string &base_path)
    {
        init(fds_config, base_path);
    }

    /**
     *
     * @param fds_config FdsConfig object
     * @param base_path base path
     */
    void init(const boost::shared_ptr<FdsConfig> &fds_config,
            const std::string &base_path)
    {
        fds_config_ = fds_config;
        base_path_ = base_path;
    }
    /**
     *
     * @return FdsConfig object
     */
    boost::shared_ptr<FdsConfig> get_fds_config() const
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
    inline void set_base_path(const std::string &base) {
        base_path_ = base;
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

    /**
     * @see FdsConfig::get(const std::string &key)
     * @param key - config key with the basepath
     * @return
     */
    template<class T>
    T get_abs(const std::string &key)
    {
        return fds_config_->get<T>(key);
    }

    /**
     * @see FdsConfig::get(const std::string &key, const T &default_value)
     * @param key - config key with the basepath
     * @param default_value
     * @return
     */
    template<class T>
    T get_abs(const std::string &key, const T &default_value)
    {
        return fds_config_->get<T>(key, default_value);
    }

    /**
     * Return true if key exists in the config
     * @param key
     */
    bool exists(const std::string &key)
    {
        return fds_config_->exists(base_path_+key);
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
    fds_spinlock::scoped_lock l(lock_);
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
    fds_spinlock::scoped_lock l(lock_);
    if (!config_.lookupValue(key, val)) {
        return default_value;
    }
    return val;
}

/**
 *
 * @param key key key is the path of the key to get.  Key is '.' separated. Key
 * must exist already.
 * @param value value to set 
 */
template<class T>
void FdsConfig::set(const std::string &key, const T &value)
{
    fds_spinlock::scoped_lock l(lock_);
    libconfig::Setting &s = config_.lookup(key);
    s = value;
}


} // namespace Fds

#endif
