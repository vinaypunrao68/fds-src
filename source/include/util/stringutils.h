/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#ifndef SOURCE_INCLUDE_UTIL_STRINGUTILS_H_
#define SOURCE_INCLUDE_UTIL_STRINGUTILS_H_
#include <string>
#include <vector>
namespace fds {
namespace util {

/**
 * printf like formatting for std::string
 */
std::string strformat(const std::string fmt_str, ...);

/**
 * lower case std::string
 */
std::string strlower(const std::string& str);

/**
 * Get a copy of the string as c cstring
 * The caller is responsible for freeing the 
 * returned value
 */
char* cstr(const std::string& str);

/**
 * Tokenize a given string into a vector of tokens
 * split at the specified by the delimiter
 */
void tokenize(const std::string& data, std::vector<std::string>& tokens, char delim = ' ');

/**
 * Join a vector of strings to a single string using the given delim
 */
std::string join(const std::vector<std::string>& tokens, char delim = ' ');

template <class T> std::string join (T const& collection, std::string const& delimiter);

/**
 * return true if the value is "true/yes/on/set/enabled/1"
 */
bool boolean(const std::string& s);
}  // namespace util
}  // namespace fds

#endif  // SOURCE_INCLUDE_UTIL_STRINGUTILS_H_
