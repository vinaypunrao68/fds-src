/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_NET_NET_UTILS_H_
#define SOURCE_INCLUDE_NET_NET_UTILS_H_

#include <string>

namespace fds {
namespace net {

std::string get_my_hostname();
std::string get_local_ip(std::string ifc);
std::string ipAddr2String(int ipaddr);
int ipString2Addr(std::string ipaddr_str);
}  // namespace net
}  // namespace fds

#endif  // SOURCE_INCLUDE_NET_NET_UTILS_H_
