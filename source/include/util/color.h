/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_UTIL_COLOR_H_
#define SOURCE_INCLUDE_UTIL_COLOR_H_

#include <string>
namespace fds {
namespace util {

struct Color {
    static std::string End;

    static std::string Black;
    static std::string Red;
    static std::string Green;
    static std::string Yellow;
    static std::string Blue;
    static std::string Magenta;
    static std::string Cyan;
    static std::string White;

    static std::string BoldBlack;
    static std::string BoldRed;
    static std::string BoldGreen;
    static std::string BoldYellow;
    static std::string BoldBlue;
    static std::string BoldMagenta;
    static std::string BoldCyan;
    static std::string BoldWhite;
};

}  // namespace util
}  // namespace fds
#endif  // SOURCE_INCLUDE_UTIL_COLOR_H_
