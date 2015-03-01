/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_UTIL_PROPERTIES_H_
#define SOURCE_INCLUDE_UTIL_PROPERTIES_H_
#include <string>
#include <map>
namespace fds {
namespace util {
struct Properties {
    bool hasValue(const std::string& key);

    bool set(const std::string& key, const std::string& value);
    bool set(const std::string& key, const long long value);
    bool set(const std::string& key, const double value);
    bool set(const std::string& key, const bool value);

    std::string get(const std::string& key, const std::string defaultValue = "");
    bool getBool(const std::string& key, const bool defaultValue=false);
    long long getInt(const std::string& key, const long long defaultValue = 0);
    double getDouble(const std::string& key, const double value = 0.0);
    const std::map<std::string, std::string>& getAllProperties();
  protected:
    std::map<std::string, std::string> data;
};
}  // namespace util
}  // namespace fds

#endif  // SOURCE_INCLUDE_UTIL_PROPERTIES_H_
