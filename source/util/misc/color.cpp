/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <util/color.h>
#include <string>

namespace fds {
namespace util {
std::string Color::End        = "\x1b[0m";

std::string Color::Black      = "\x1b[30m";
std::string Color::Red        = "\x1b[31m";
std::string Color::Green      = "\x1b[32m";
std::string Color::Yellow     = "\x1b[33m";
std::string Color::Blue       =  "\x1b[34m";
std::string Color::Magenta    = "\x1b[35m";
std::string Color::Cyan       =  "\x1b[36m";
std::string Color::White      = "\x1b[37m";

std::string Color::BoldBlack  = "\x1b[1;30m";
std::string Color::BoldRed    = "\x1b[1;31m";
std::string Color::BoldGreen  = "\x1b[1;32m";
std::string Color::BoldYellow = "\x1b[1;33m";
std::string Color::BoldBlue   =  "\x1b[1;34m";
std::string Color::BoldMagenta= "\x1b[1;35m";
std::string Color::BoldCyan   =  "\x1b[1;36m";
std::string Color::BoldWhite  = "\x1b[1;37m";
}  // namespace util
}  // namespace fds
