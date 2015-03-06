/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

#include <util/stringutils.h>
#include <unistd.h>
#include <stdarg.h>
#include <memory>
#include <string>
#include <cstring>
#include <algorithm>
#include <sstream>
namespace fds {
namespace util {

std::string strlower(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    return lower;
}

std::string strformat(const std::string fmt_str, ...) {
    // http://stackoverflow.com/a/8098080/2643320
    /* reserve 2 times as much as the length of the fmt_str */
    int final_n, n = ((int)fmt_str.size()) * 2; //NOLINT
    std::string str;
    std::unique_ptr<char[]> formatted;
    va_list ap;
    while (1) {
        formatted.reset(new char[n]); /* wrap the plain char array into the unique_ptr */
        std::strcpy(&formatted[0], fmt_str.c_str()); //NOLINT
        va_start(ap, fmt_str);
        final_n = vsnprintf(&formatted[0], n, fmt_str.c_str(), ap);
        va_end(ap);
        if (final_n < 0 || final_n >= n)
            n += abs(final_n - n + 1);
        else
            break;
    }
    return std::string(formatted.get());
}

char* cstr(const std::string& str) {
    char* newstr = new char[str.size() + 1];
    std::copy(str.begin(), str.end(), newstr);
    newstr[str.size()] = '\0';
    return newstr;
}

void tokenize(const std::string& data, std::vector<std::string>& tokens, char delim) {
    std::istringstream f(data);
    std::string s;
    while (getline(f, s, delim)) {
        tokens.push_back(s);
    }
}

std::string join(const std::vector<std::string>& tokens, char delim) {
    std::ostringstream oss;
    for(size_t i = 0; i< tokens.size() ; i++) {
        oss << tokens[i];
        if (i < (tokens.size() - 1)) {
            oss << delim;
        }
    }
    return oss.str();
}

bool boolean(const std::string& s) {
    const char* p =s.c_str();
    return ( 
        (0 == strcasecmp(p, "enable")) ||
        (0 == strcasecmp(p, "true")) ||
        (0 == strcasecmp(p, "yes")) ||
        (0 == strcasecmp(p, "set")) ||
        (0 == strcasecmp(p, "on")) ||
        (0 == strcasecmp(p, "1")));
}

}  // namespace util
}  // namespace fds
