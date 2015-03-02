/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <util/properties.h>
#include <map>
namespace fds {
namespace util {

#define RETURN_DEFAULT(...) \
    auto iter = data.find(key); \
    if (iter == data.end()) return defaultValue;

bool Properties::hasValue(const std::string& key) {
    auto iter = data.find(key);
    return iter != data.end();
}

bool Properties::set(const std::string& key, const std::string& value) {
    auto iter = data.find(key);
    data[key] = value;
    return (iter == data.end());
}

bool Properties::set(const std::string& key, const long long value) {
    auto iter = data.find(key);
    data[key] = std::to_string(value);
    return (iter == data.end());

}

bool Properties::set(const std::string& key, const double value) {
    auto iter = data.find(key);
    data[key] = std::to_string(value);
    return (iter == data.end());
}

bool Properties::set(const std::string& key, const bool value) {
    auto iter = data.find(key);
    if (value) data[key] = "1";
    else data[key] = "0";
    return (iter == data.end());
}

std::string Properties::get(const std::string& key, const std::string defaultValue) {
    RETURN_DEFAULT();
    return iter->second;
}

bool Properties::getBool(const std::string& key, const bool defaultValue) {
    RETURN_DEFAULT();
    if (iter->second.empty()) return defaultValue;
    if (iter->second.compare("yes") == 0 
        || iter->second.compare("ok") == 0
        || iter->second.compare("1") == 0
        || iter->second.compare("true") == 0
        || iter->second.compare("set") == 0
        ) return true;
    return false;
}

long long Properties::getInt(const std::string& key, const long long defaultValue) {
    RETURN_DEFAULT();
    return std::stoll(iter->second, nullptr, 10);
}

double Properties::getDouble(const std::string& key, const double defaultValue) {
    RETURN_DEFAULT();
    return std::stod(iter->second, nullptr);
}

const std::map<std::string, std::string>& Properties::getAllProperties() {
    return data;
}
}  // namespace util
}  // namespace fds
