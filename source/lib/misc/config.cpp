/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <fds_config.hpp>
#include <util/stringutils.h>
namespace fds {
using libconfig::Setting;
// helper funcs
Setting& getHierarchialValue(const libconfig::Config& config, const std::string& key) {
    if (!config.exists(key)) throw fds::Exception(key + " not found");
    // TODO(prem) : fill in the function properly
    return config.lookup(key);
}

Setting& add(libconfig::Config& config, const std::string& key, Setting::Type type) {
    Setting* s = &(config.getRoot());
    std::vector<std::string> tokens;
    util::tokenize(key, tokens, '.');

    if (tokens.size() == 0) throw fds::Exception("config key cannot be empty");
    
    for (size_t i = 0; i < (tokens.size()-1); i++) {
        if (s->exists(tokens[i])) {
            s = &((*s)[tokens[i]]);
            if (s->getType() != Setting::TypeGroup) throw fds::Exception("existing key is not a group : " + key);
        } else {
            s = &(s->add(tokens[i], Setting::TypeGroup));
        }
    }
    size_t pos = tokens.size()-1;

    // now the final part
    return s->add(tokens[pos], type);
}

void remove(libconfig::Config& config, const std::string& key) {
    if (!config.exists(key)) return;
    size_t pos = key.rfind('.');
    std::string keyname;
    Setting* s;
    if (pos == std::string::npos){
        s = &(config.getRoot());
        keyname=key;
    } else {
        s = &(config.lookup(key.substr(0, pos)));
        keyname=key.substr(pos+1);
    }
    s->remove(keyname);
}


FdsConfig::FdsConfig() {
}

FdsConfig::FdsConfig(const std::string &default_config_file, int argc, char *argv[]) : lock_("FdsConfig") {
    init(default_config_file, argc, argv);
}

void FdsConfig::init(const std::string &default_config_file, int argc, char* argv[]) {
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

    /* Check if config file exists */
    struct stat buf;
    if (stat(config_file.c_str(), &buf) == -1) {
        std::cout << "Configuration file " << config_file << " not found. Exiting."
        			<< std::endl;
        exit(ERR_DISK_READ_FAILED);
    }
    config_.readFile(config_file.c_str());

    /* Override config read from with command line params */
    for (auto o : parsed.options) {
        /* Ignore options that don't start with "fds." */
        if (o.string_key.find("fds.") != 0) {
            continue;
        }

        if (!config_.exists(o.string_key)) {
            // if the config does not exist, then add it
            Setting& s = add(config_, o.string_key, Setting::TypeString);
            s = o.value[0];
            continue;
        }
        set(o.string_key, o.value[0]);
    }
}

bool FdsConfig::exists(const std::string &key) {
    fds_mutex::scoped_lock l(lock_);
    return config_.exists(key);
}

template<> 
std::string  FdsConfig::get<std::string> (const std::string &key) {
    fds_mutex::scoped_lock l(lock_);
    Setting &s = getHierarchialValue(config_, key);
    switch (s.getType()) {
        case Setting::TypeInt64:
            return std::to_string((long long) s);
        case Setting::TypeInt:
            return std::to_string((int) s);
        case Setting::TypeString:
            return (const char*) s;
        case Setting::TypeBoolean:
            return ((bool) s?"true":"false");
        case Setting::TypeFloat:
            return std::to_string((double)s);
        default:
            throw fds::Exception("Unsupported type for key: " + key);
    }
}

template<> 
fds_int32_t  FdsConfig::get<fds_int32_t> (const std::string &key) {
    fds_mutex::scoped_lock l(lock_);        
    Setting &s = getHierarchialValue(config_, key);
    switch (s.getType()) {
        case Setting::TypeInt64:
            return (long long) s;
        case Setting::TypeInt:
            return (int) s;
        case Setting::TypeString:
            return std::stoi((const char*)s,nullptr,0);
        case Setting::TypeBoolean:
            return (bool) s?1:0;
        case Setting::TypeFloat:
            return (double) s;
        default:
            throw fds::Exception("Unsupported type for key: " + key);
    }
}

template<> 
fds_uint32_t FdsConfig::get<fds_uint32_t>(const std::string &key) {
    return (fds_uint32_t) get<fds_int32_t>(key);
}

template<> fds_bool_t FdsConfig::get<fds_bool_t>  (const std::string &key) {
    fds_mutex::scoped_lock l(lock_);        
    Setting &s = getHierarchialValue(config_, key);
    switch (s.getType()) {
        case Setting::TypeInt64:
            return  0 != (long long) s;
        case Setting::TypeInt:
            return  0 != (int) s;
        case Setting::TypeString:
            return util::boolean((const char*) s);
        case Setting::TypeBoolean:
            return (bool) s;
        case Setting::TypeFloat:
            return 0.0 != (double) s;                                 
        default:
            throw fds::Exception("Unsupported type for key: " + key);
    }
}

template<> 
fds_int64_t FdsConfig::get<fds_int64_t>(const std::string &key) {
    fds_mutex::scoped_lock l(lock_);        
    Setting &s = getHierarchialValue(config_, key);
    switch (s.getType()) {
        case Setting::TypeInt64:
            return (long long) s;
        case Setting::TypeInt:
            return (int) s;
        case Setting::TypeString:
            return std::stoll((const char*)s,nullptr,0);
            break;
        default:
            throw fds::Exception("Unsupported type for key: " + key);
    }
}

template<>
fds_uint64_t FdsConfig::get<fds_uint64_t>(const std::string &key) {
    return (fds_uint64_t)get<fds_int64_t>(key);
}

template<>
double FdsConfig::get<double> (const std::string &key) {
    fds_mutex::scoped_lock l(lock_);        
    Setting &s = getHierarchialValue(config_, key);
    switch (s.getType()) {
        case Setting::TypeInt64:
            return (long long) s;
        case Setting::TypeInt:
            return (int) s;
        case Setting::TypeString:
            return std::stod((const char*)s,nullptr);
            break;
        default:
            throw fds::Exception("Unsupported type for key: " + key);
    }
}

void FdsConfig::set(const std::string &key, const char * value) {
    set(key, std::string(value));
}

void FdsConfig::set(const std::string &key, const std::string& value) {
    fds_mutex::scoped_lock l(lock_);
    if (config_.exists(key)) {
        Setting& s = config_.lookup(key);
        switch (s.getType()) {
            case Setting::TypeInt64:
                s = std::stoll(value, NULL);
                break;
            case Setting::TypeInt:
                try {
                    s = std::stoi(value, NULL);
                } catch(std::out_of_range& e) {
                    std::cout << "int cast failed.. trying int64 : " << key <<std::endl;
                    remove(config_, key);
                    Setting& s1 = add(config_, key, Setting::TypeInt64);
                    s1 = std::stoll(value, NULL);
                }
                break;
            case Setting::TypeString:
                s = value;
                break;
            case Setting::TypeBoolean:
                s = util::boolean(value);
                break;
            break;
        default:
            throw fds::Exception("Unsupported type for key: " + key);
        }
    } else {
        Setting& s = add(config_, key, Setting::TypeString);
        s = value;
    }
}

#define SETDATA(OP,TYPE)                                 \
    fds_mutex::scoped_lock l(lock_);                  \
    if (config_.exists(key)) {                           \
        Setting& s = config_.lookup(key);                \
        s = (OP) value;                                  \
    } else {                                             \
        Setting& s = add(config_, key, Setting::TYPE);   \
        s = (OP) value;                                  \
    }

void FdsConfig::set(const std::string &key, const fds_int32_t& value) {
    SETDATA(int, TypeInt);
}

void FdsConfig::set(const std::string &key, const fds_uint32_t& value) {
    SETDATA(int, TypeInt);
}

void FdsConfig::set(const std::string &key, const fds_bool_t& value) {
    SETDATA(bool, TypeBoolean);
}

void FdsConfig::set(const std::string &key, const fds_uint64_t& value) {
    SETDATA(long long, TypeInt64);
}

void FdsConfig::set(const std::string &key, const fds_int64_t& value) {
    SETDATA(long long, TypeInt64);
}

void FdsConfig::set(const std::string &key, const double& value) {
    SETDATA(double, TypeFloat);
}


/**********************************************************************
 *   FdsConfigAccessor
 **********************************************************************/
FdsConfigAccessor::FdsConfigAccessor() {
}

FdsConfigAccessor::FdsConfigAccessor(const boost::shared_ptr<FdsConfig> &fds_config,
                  const std::string &base_path) {
    init(fds_config, base_path);
}

void FdsConfigAccessor::init(const boost::shared_ptr<FdsConfig> &fds_config,
                             const std::string &base_path) {
    fds_config_ = fds_config;
    base_path_ = base_path;
}

const boost::shared_ptr<FdsConfig> FdsConfigAccessor::get_fds_config() const {
    return fds_config_;
}

std::string FdsConfigAccessor::get_base_path() {
    return base_path_;
}

void FdsConfigAccessor::set_base_path(const std::string &base) {
    base_path_ = base;
}

bool FdsConfigAccessor::exists(const std::string &key) {
    return fds_config_->exists(base_path_+key);
}

}  // namespace fds
