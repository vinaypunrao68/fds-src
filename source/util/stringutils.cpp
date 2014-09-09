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

}  // namespace util
}  // namespace fds
